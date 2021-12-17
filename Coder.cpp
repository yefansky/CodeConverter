#include "stdafx.h"
#include "Coder.h"
#include "CommonWords.h"
#include "FileList.h"

struct BOMDesc
{
	char szBOM[4];
	size_t nLen;
	const char* cpszName;
};

static BOMDesc s_BomDesc[] = { // 除了UTF8，还可能出现多种不同编码的BOM
	{{(char)0xEF, (char)0xBB, (char)0xBF}, 3, "UTF-8"},
	{{(char)0xFE, (char)0xFF}, 2, "UTF-16BE"}, //  "UTF-16 Big-Endian"
	{{(char)0xFF, (char)0xFE}, 2, "UTF-16LE"}, // "UTF-16 Little-Endian"
	{{(char)0x00, (char)0x00, (char)0xFE, (char)0xFF}, 4, "UTF-32BE"}, // "UTF-32 Big-Endian"
	{{(char)0xFF, (char)0xFE, (char)0x00, (char)0x00}, 4, "UTF-32LE"}, // "UTF-32 Little-Endian"
	{{(char)0x2B, (char)0x2F, (char)0x76, (char)0x38}, 4, "UTF-7"}, // "UTF-7"
	{{(char)0x2B, (char)0x2F, (char)0x76, (char)0x39}, 4, "UTF-7"}, // "UTF-7"
	{{(char)0x2B, (char)0x2F, (char)0x76, (char)0x2B}, 4, "UTF-7"}, // "UTF-7"
	{{(char)0x2B, (char)0x2F, (char)0x76, (char)0x2F}, 4, "UTF-7"}, // "UTF-7"
	{{(char)0xF7, (char)0x64, (char)0x4C}, 3, "en:UTF-1"},
	{{(char)0xDD, (char)0x73, (char)0x66, (char)0x73}, 4, "en:UTF-EBCDIC"},
	{{(char)0x0E, (char)0xFE, (char)0xFF}, 3, "en:Standard Compression Scheme for Unicode"},
	{{(char)0xFB, (char)0xEE, (char)0x28}, 3, "en:BOCU-1"},
	{{(char)0xFB, (char)0xEE, (char)0x28, (char)0xFF}, 4, "en:BOCU-1"},
	{{(char)0x84, (char)0x31, (char)0x95, (char)0x33}, 4, "GB-18030"}
};

bool Coder::LoadFile(const char* pszPath)
{
	bool bResult = false;
	FILE* pfFile = nullptr;
	size_t uReadSize = 0;

	assert(pszPath);

	m_strPath = pszPath;

	pfFile = fopen(pszPath, "rb"); // 为了正确校验文件大小，必须用二进制读取
	KGLOG_PROCESS_ERROR(pfFile);

	if (m_pszBuffer)
	{
		delete[] m_pszBuffer;
		m_pszBuffer = nullptr;
		m_uFileSize = 0;
	}

	fseek(pfFile, 0, SEEK_END);
	m_uFileSize = ftell(pfFile);

	fseek(pfFile, 0, SEEK_SET);
	KGLOG_PROCESS_ERROR(m_uFileSize >= 0);

	if (m_uFileSize > 0)
	{
		m_pszBuffer = new char[m_uFileSize];

		uReadSize = fread(m_pszBuffer, 1, m_uFileSize, pfFile);
		KGLOG_PROCESS_ERROR(uReadSize == m_uFileSize);
	}

	bResult = true;
Exit0:
	if (pfFile)
	{
		fclose(pfFile);
		pfFile = nullptr;
	}
	if (!bResult)
	{
		Release();
	}
	return bResult;
}

void Coder::Release()
{
	KG_DELETE_ARRAY(m_pszBuffer);
	m_uFileSize = 0;
	m_strPath = "";
}

int IsASCII(const char* ch , size_t uLeftSize = 1)
{
	assert(ch);
	if (*ch >= 0 && *ch <= 0x7F)
		return 1;
	return 0;
}

int IsGBK(const char* ch, size_t uLeftSize)
{
	if (IsASCII(ch))
		return 1;
	if (uLeftSize < 2)
		return 0;
	unsigned char ch1 = *ch;
	unsigned char ch2 = *(ch + 1);
	if (
			(ch1 >= 0xA1 && ch1 <= 0xA9 && ch2 >= 0xA1 && ch2 <= 0xFE) ||
			(ch1 >= 0xB0 && ch1 <= 0xF7 && ch2 >= 0xA1 && ch2 <= 0xFE) ||
			(ch1 >= 0x81 && ch1 <= 0xA0 && ch2 >= 0x40 && ch2 <= 0xFE && ch2 != 0x7F) ||
			(ch1 >= 0xAA && ch1 <= 0xFE && ch2 >= 0x40 && ch2 <= 0xA0 && ch2 != 0x7F) ||
			(ch1 >= 0xA8 && ch1 <= 0xA9 && ch2 >= 0x40 && ch2 <= 0xA0 && ch2 != 0x7F)
			// 		(ch1 >= 0xAA && ch1 <= 0xAF && ch2 >= 0xA1 && ch2 <= 0xFE) || // 自定义段，应该不会出现
			// 		(ch1 >= 0xF8 && ch1 <= 0xFE && ch2 >= 0xA1 && ch2 <= 0xFE) ||
			// 		(ch1 >= 0xA1 && ch1 <= 0xA7 && ch2 >= 0x40 && ch2 <= 0xA0 && ch2 != 0x7F)
		)
		return 2;
	return 0;
}

int IsUTF8(const char* ch, size_t uLeftSize)
{
	static const unsigned char s_cuMask = 1 << 7;
	int nLen = 0;
	for (int i = 0; i < 7; i++) // UTF8 第一个字节 用打头的二进制 '1'个数表示一共有几个字节， 除了ASCII是0打头的
	{
		if (*ch & (s_cuMask >> i))
			nLen++;
		else
			break;
	}

	if (nLen == 0)
		return IsASCII(ch);

	if (nLen < 2) // 除了ASCII，后续至少2个字节
		return 0;

	if (nLen > 6) // UTF8的最大长度不会超过这个上限
		return 0;

	if (nLen != 3) // 这是一个假设，因为大部分的中文落在UTF8的3个字节区域
		return 0;

	if (uLeftSize < nLen)
		return 0;

	static const unsigned char s_cuSuffixHeader = 1 << 7;
	for (int i = 1; i < nLen; i++)
	{
		if ((((unsigned char)*(ch + i)) >> 6 << 6) != s_cuSuffixHeader) // 从第二个字开始，都是用 10 开头的
			return 0;
	}
	return nLen;
}

int _IsUTF8(const char* ch, size_t uLeftSize)
{
	static const unsigned char s_cuMask = 1 << 7;
	int nLen = 0;
	for (int i = 0; i < 7; i++) // UTF8 第一个字节 用打头的二进制 '1'个数表示一共有几个字节， 除了ASCII是0打头的
	{
		if (*ch & (s_cuMask >> i))
			nLen++;
		else
			break;
	}

	if (nLen == 0)
		return IsASCII(ch);

	if (nLen < 2) // 除了ASCII，后续至少2个字节
		return 0;

	if (nLen > 6) // UTF8的最大长度不会超过这个上限
		return 0;

	if (uLeftSize < nLen)
		return 0;

	static const unsigned char s_cuSuffixHeader = 1 << 7;
	for (int i = 1; i < nLen; i++)
	{
		if ((((unsigned char)*(ch + i)) >> 6 << 6) != s_cuSuffixHeader) // 从第二个字开始，都是用 10 开头的
			return 0;
	}
	return nLen;
}



bool CheckForCode(const char* pszBuffer, size_t uSize, int (*checker)(const char* ch, size_t uLeftSize))
{
	const char* pszOffset = pszBuffer;
	size_t uLeftSize = uSize;

	assert(pszBuffer);
	assert(checker);

	while (uLeftSize)
	{
		int nLen = checker(pszOffset, uLeftSize);
		if (nLen == 0)
		{
			return false;
			break;
		}
		pszOffset += nLen;
		uLeftSize -= nLen;
	}
	return true;
}

bool GetStopPos(const char* pszBuffer, size_t uSize, int (*checker)(const char* ch, size_t uLeftSize), int* pnRetLine, int* pnRetCol, int* pnRetOffset)
{
	bool bResult = false;
	const char* pszOffset = pszBuffer;
	size_t uLeftSize = uSize;
	int nLineCounter = 1;
	int nColCounter = 0;
	int nOffset = 0;

	assert(pszBuffer);
	assert(checker);

	while (uLeftSize)
	{
		int nLen = checker(pszOffset, uLeftSize);

		nColCounter++;
		nOffset += nLen;

		if (nLen == 0)
		{
			bResult = true;
			break;
		}

		if (*pszOffset == '\n')
		{
			nLineCounter++;
			nColCounter = 0;
		}

		pszOffset += nLen;
		uLeftSize -= nLen;
	}

	if (pnRetLine)
		*pnRetLine = nLineCounter;
	if (pnRetCol)
		*pnRetCol = nColCounter;
	if (pnRetOffset)
		*pnRetOffset = nOffset;

//Exit0:
	return bResult;
}

void Coder::ShowConfusionPos(const char* pszBuffer, size_t uSize, size_t uBOMLen)
{
	bool	bRetCode		= false;
	int		nLineCounter	= 0;
	int		nColCounter		= 0;
	int		nOffset			= 0;

	assert(pszBuffer);
	assert(uSize > 0);

	SetConsoleColor(FOREGROUND_RED);

	bRetCode = GetStopPos(pszBuffer, uSize, IsGBK, &nLineCounter, &nColCounter, &nOffset);
	if (bRetCode)
	{
		printf("GBK stop at line:%d col:%d, offset:0x%x(%d)\n", nLineCounter, nColCounter, (unsigned)(nOffset + uBOMLen), (int)(nOffset + uBOMLen));

		if (nOffset <= uSize - 2)
		{
			char szTry[4] = { pszBuffer[nOffset], pszBuffer[nOffset + 1],  '\0' };
			putchar('\t');
			ShowHex(pszBuffer + nOffset, 2);
			ShowBinary(pszBuffer + nOffset, 2);
			putchar('\n');
		}
		else
		{
			putchar('\t');
			printf("file end incomplete: 0x%02x", (unsigned char)pszBuffer[nOffset]);
		}
	}

	bRetCode = GetStopPos(pszBuffer, uSize, IsUTF8, &nLineCounter, &nColCounter, &nOffset);
	if (bRetCode)
	{
		printf("UTF8 stop at line:%d col:%d, offset:0x%x(%d)\n", nLineCounter, nColCounter, (unsigned)(nOffset + uBOMLen), (int)(nOffset + uBOMLen));

		if (nOffset <= uSize - 3)
		{
			char szTry[4] = { pszBuffer[nOffset], pszBuffer[nOffset + 1], pszBuffer[nOffset + 2], '\0' };
			putchar('\t');
			ShowHex(pszBuffer + nOffset, 3);
			ShowBinary(pszBuffer + nOffset, 3);
			putchar('\n');
			putchar('\t');
			TryOutputUTF8((int)(nOffset + uBOMLen));
		}
		else
		{
			putchar('\t');
			printf("file end incomplete: 0x%02x", (unsigned char)pszBuffer[nOffset]);
		}
	}

	bRetCode = GetStopPos(pszBuffer, uSize, IsASCII, &nLineCounter, &nColCounter, &nOffset);
	if (bRetCode)
	{
		printf("ASCII stop at line:%d col:%d, offset:0x%x(%d)\n", nLineCounter, nColCounter, (unsigned)(nOffset + uBOMLen), (int)(nOffset + uBOMLen));
		putchar('\t');
		ShowHex(pszBuffer + nOffset, 3);
		ShowBinary(pszBuffer + nOffset, 3);
		putchar('\n');

		if (nOffset < uSize)
		{
			char szTry[4] = { pszBuffer[nOffset], pszBuffer[nOffset + 1], '\0' };
			putchar('\t');
			TryOutputGBK((int)(nOffset + uBOMLen));
			putchar('\t');
			TryOutputUTF8((int)(nOffset + uBOMLen));
		}
		else
		{
			putchar('\t');
			printf("file end incomplete: 0x%02x", (unsigned char)pszBuffer[nOffset]);
		}
	}

	return;
}

void Coder::ShowBinary(const char* pszBuffer, size_t uLen)
{
	assert(pszBuffer);

	printf("Binary:");
	for (int i = 0; i < uLen; i++)
	{
		unsigned char uCh = (unsigned char)*(pszBuffer + i);
		for (int j = 0; j < CHAR_BIT; j++)
			putchar((uCh & ((1u << 7) >> j)) ? '1' : '0');
		putchar(' ');
	}
}

void Coder::ShowHex(const char* pszBuffer, size_t uLen)
{
	assert(pszBuffer);

	printf("Hex: 0x");
	for (int i = 0; i < uLen; i++)
	{
		unsigned char uCh = (unsigned char)*(pszBuffer + i);
		printf("%02X", uCh);
		putchar(' ');
	}
}

bool Coder::Check()
{
	bool					bResult		= false;
	bool					bRetCode	= false;
	int						nOffset		= 0;
	bool					bHaveBom	= false;
	bool					bASICCPass	= false;
	bool					bGBKPass	= false;
	bool					bUTF8Pass	= false;
	const BOMDesc*			pBOMDesc	= nullptr;
	static const BOMDesc*	s_cpUTF8Bom = &s_BomDesc[0];

	assert(m_uFileSize >= 0);

	KG_PROCESS_SUCCESS(m_uFileSize == 0);

	assert(m_pszBuffer);

	pBOMDesc = GetBOMType();

	if (pBOMDesc)
	{
		bHaveBom = true;

		KG_PROCESS_SUCCESS(pBOMDesc != s_cpUTF8Bom);

		bUTF8Pass = CheckForCode(m_pszBuffer + pBOMDesc->nLen, m_uFileSize - pBOMDesc->nLen, IsUTF8);
		KG_PROCESS_ERROR(bUTF8Pass);
	}
	else
	{
		bASICCPass = CheckForCode(m_pszBuffer, m_uFileSize, IsASCII);
		bGBKPass = CheckForCode(m_pszBuffer, m_uFileSize, IsGBK);
		bUTF8Pass = CheckForCode(m_pszBuffer, m_uFileSize, IsUTF8);

		if (bGBKPass && bUTF8Pass && !bASICCPass)
		{
			bRetCode = GetStopPos(m_pszBuffer, m_uFileSize, IsASCII, nullptr, nullptr, &nOffset);
			assert(bRetCode);
			assert(nOffset < m_uFileSize);
			if (!IsCommonGBKWord(m_pszBuffer[nOffset], m_pszBuffer[nOffset + 1]) ||
				IsUnommonGBKWord(m_pszBuffer[nOffset], m_pszBuffer[nOffset + 1]))
				bGBKPass = false;
		}

		KG_PROCESS_ERROR((bGBKPass || bUTF8Pass) && !(bGBKPass && bUTF8Pass && !bASICCPass));
	}

Exit1:
	bResult = true;
Exit0:
	if (!bResult)
	{
		SetConsoleColor c(FOREGROUND_RED);

		if (bHaveBom)
		{
			if (m_bOutputBOMMissMatch)
			{
				if (pBOMDesc != s_cpUTF8Bom)
				{
					printf("[ERROR] bom type is %s : %s\n", pBOMDesc->cpszName, m_strPath.c_str());
				}
				else if (!bUTF8Pass)
				{
					printf("[ERROR] have utf8 bom, but content not utf8: %s\n", m_strPath.c_str());
					ShowConfusionPos(m_pszBuffer + s_cpUTF8Bom->nLen, m_uFileSize - s_cpUTF8Bom->nLen, s_cpUTF8Bom->nLen);
				}
				else
				{
					assert(0);
				}
			}
		}
		else 
		{
			if (m_bOutputUnrecognize)
			{
				printf("[ERROR] Unrecognized: %s\n", m_strPath.c_str());
				if (bUTF8Pass)
					printf("maybe utf8\n");
				if (bGBKPass)
					printf("maybe gbk\n");
				if (!bUTF8Pass && !bGBKPass)
					printf("not utf8 and gbk\n");
				ShowConfusionPos(m_pszBuffer, m_uFileSize, 0);
			}
		}
	}
	else
	{
		if (m_bOutputAllFileType)
		{
			if (bUTF8Pass)
			{
				SetConsoleColor c(FOREGROUND_BLUE);
				printf("[UTF8] %s\n", m_strPath.c_str());
			}
			else if (bGBKPass)
			{
				SetConsoleColor c(FOREGROUND_GREEN);
				printf("[GBK] %s\n", m_strPath.c_str());
			}
			else if (m_uFileSize == 0)
			{
				SetConsoleColor c(FOREGROUND_GREEN);
				printf("[EMPTY] %s\n", m_strPath.c_str());
			}
			else
			{
				assert(0);
			}
		}

		if (m_bOutputConvertScript)
		{
			char szPath[MAX_PATH];
			strncpy(szPath, m_strPath.c_str(), sizeof(szPath));
			szPath[sizeof(szPath) - 1] = 0;

			bRetCode = FileList::ConvertToLinuxPath(szPath, sizeof(szPath));
			assert(bRetCode);

			if (!m_pfFile)
			{
				m_pfFile = fopen("ConvertCoding.sh", "wb");
				fprintf(m_pfFile, "#!/bin/bash\n");
				fprintf(m_pfFile,
					"\n\n"
					"function Convert() {\n"
					"		path=\"$1\"\n"
					"		fcode=\"$2\"\n"
					"		tcode=\"$3\"\n"
					"		iconv -f $fcode -t $tcode \"$path\" -o tmp\n"
					"		if [ $? -ne 0 ] ; then\n"
					"			echo \"error $path\"\n"
					"			return\n"
					"		fi\n"
					"		mv tmp \"$path\"\n"
					"}\n"
					"\n"
					"function AddBom() {\n"
					"	path=\"$1\"\n"
					"	sed -i '1 s/^/\\xEF\\xBB\\xBF/' \"$path\"\n"
					"}\n"
					"\n"
				);
			}
			assert(m_pfFile);

			if (!bHaveBom)
			{
				if (bUTF8Pass)
				{
					fprintf(m_pfFile, "AddBom \"%s\"\n", szPath);
				}
				else if (bGBKPass)
				{
					fprintf(m_pfFile, "Convert \"%s\" gb18030 utf-8\n", szPath);
					fprintf(m_pfFile, "AddBom \"%s\"\n", szPath);
				}
			}
			else if (_stricmp(pBOMDesc->cpszName, "UTF-16BE") == 0 ||
				_stricmp(pBOMDesc->cpszName, "UTF-16LE") == 0 ||
				_stricmp(pBOMDesc->cpszName, "UTF-32BE") == 0 ||
				_stricmp(pBOMDesc->cpszName, "UTF-32LE") == 0
				)
			{
				fprintf(m_pfFile, "Convert \"%s\" %s utf-8\n", szPath, pBOMDesc->cpszName);
			}
			else if (pBOMDesc != s_cpUTF8Bom && m_bOutputBOMMissMatch)
			{
				printf("[ERROR] bom type is %s : %s\n", pBOMDesc->cpszName, m_strPath.c_str());
			}
		}
	}
	return bResult;
}

BOMDesc* Coder::GetBOMType()
{
	BOMDesc* pResult = nullptr;

	assert(m_pszBuffer && m_uFileSize > 0);

	for (int i = 0; i < _countof(s_BomDesc); i++)
	{
		auto& rDesc = s_BomDesc[i];
		if (m_uFileSize < rDesc.nLen)
			continue;

		bool bMatch = true;
		for (int j = 0; j < rDesc.nLen; j++)
		{
			if (m_pszBuffer[j] != rDesc.szBOM[j])
			{
				bMatch = false;
				break;
			}
		}

		if (bMatch)
		{
			pResult = &s_BomDesc[i];
			goto Exit1;
		}
	}

Exit1:
	return pResult;
}

wchar_t UTF8toWChar(const char* pszUTF8)
{
	wchar_t wChar = 0;
	MultiByteToWideChar(CP_UTF8, NULL, pszUTF8, (int)strlen(pszUTF8), &wChar, 1);
	return wChar;
}

void Coder::TryOutputUTF8(int nOffset)
{
	int nLen = _IsUTF8(m_pszBuffer + nOffset, m_uFileSize - nOffset);
	wchar_t w = 0;

	if (nLen > 0)
	{
		char szTmp[8];
		assert(nLen < _countof(szTmp) - 1);
		for (int i = 0; i < nLen; i++)
		{
			szTmp[i] = *(m_pszBuffer + nOffset + i);
		}
		szTmp[nLen] = '\0';
		w = UTF8toWChar(szTmp);
	}
	if (w)
		wprintf(L"Try Parsing as UTF8 char: '%lc' (0x%04x)\n", w, (unsigned short)w);
	else
		wprintf(L"Try Parsing as UTF8 char failed\n");
}

void Coder::TryOutputGBK(int nOffset)
{
	int nLen = IsGBK(m_pszBuffer + nOffset, m_uFileSize - nOffset);
	assert(nLen <= 2);

	if (nLen > 0)
	{
		char szTmp[8];
		assert(nLen < _countof(szTmp) - 1);
		for (int i = 0; i < nLen; i++)
		{
			szTmp[i] = *(m_pszBuffer + nOffset + i);
		}
		szTmp[nLen] = '\0';

		printf("Try Parsing as GBK char: '%s' (0x%02x %02x)\n", szTmp, (unsigned char)szTmp[0], (unsigned char)szTmp[1]);
	}
	else
		printf("Try Parsing as GBK char failed\n");
}
