#include "stdafx.h"
#include "Coder.h"

struct BOMDesc
{
	char szBOM[4];
	size_t nLen;
	const char* cpszName;
};

static BOMDesc s_BomDesc[] = {
	{{(char)0xEF, (char)0xBB, (char)0xBF}, 3, "UTF-8"},
	{{(char)0xFE, (char)0xFF}, 2, "UTF-16 Big-Endian"},
	{{(char)0xFF, (char)0xFE}, 2, "UTF-16 Little-Endian"},
	{{(char)0x00, (char)0x00, (char)0xFE, (char)0xFF}, 4, "UTF-32 Big-Endian"},
	{{(char)0xFF, (char)0xFE, (char)0x00, (char)0x00}, 4, "UTF-32 Little-Endian"},
	{{(char)0x2B, (char)0x2F, (char)0x76, (char)0x38}, 4, "UTF-7"},
	{{(char)0x2B, (char)0x2F, (char)0x76, (char)0x39}, 4, "UTF-7"},
	{{(char)0x2B, (char)0x2F, (char)0x76, (char)0x2B}, 4, "UTF-7"},
	{{(char)0x2B, (char)0x2F, (char)0x76, (char)0x2F}, 4, "UTF-7"},
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

	pfFile = fopen(pszPath, "rb");
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
	if ((ch1 >= 0xA1 && ch1 <= 0xA9 && ch2 >= 0xA1 && ch2 <= 0xFE) ||
		(ch1 >= 0xB0 && ch1 <= 0xF7 && ch2 >= 0xA1 && ch2 <= 0xFE) ||
		(ch1 >= 0x81 && ch1 <= 0xA0 && ch2 >= 0x40 && ch2 <= 0xFE && ch2 != 0x7F) ||
		(ch1 >= 0xAA && ch1 <= 0xFE && ch2 >= 0x40 && ch2 <= 0xA0 && ch2 != 0x7F) ||
		(ch1 >= 0xA8 && ch1 <= 0xA9 && ch2 >= 0x40 && ch2 <= 0xA0 && ch2 != 0x7F)
// 		(ch1 >= 0xAA && ch1 <= 0xAF && ch2 >= 0xA1 && ch2 <= 0xFE) ||
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
	for (int i = 0; i < 7; i++)
	{
		if (*ch & (s_cuMask >> i))
			nLen++;
		else
			break;
	}

	if (nLen == 0)
		return IsASCII(ch);

	if (nLen > 6)
		return 0;

	if (uLeftSize < nLen)
		return 0;

	static const unsigned char s_cuSuffixMask = 3 << 6;
	static const unsigned char s_cuSuffixHeader = 2 << 6;
	for (int i = 1; i < nLen; i++)
	{
		if ((*(ch + i) & s_cuSuffixMask) != s_cuSuffixHeader)
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
	assert(pnRetLine);
	assert(pnRetCol);
	assert(pnRetOffset);

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

	*pnRetLine		= nLineCounter;
	*pnRetCol		= nColCounter;
	*pnRetOffset	= nOffset;

//Exit0:
	return bResult;
}

void ShowConfusionPos(const char* pszBuffer, size_t uSize)
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
		printf("GBK stop at line:%d col:%d, offset:0x%x(%d)\n", nLineCounter, nColCounter, nOffset, nOffset);

	bRetCode = GetStopPos(pszBuffer, uSize, IsUTF8, &nLineCounter, &nColCounter, &nOffset);
	if (bRetCode)
		printf("UTF8 stop at line:%d col:%d, offset:0x%x(%d)\n", nLineCounter, nColCounter, nOffset, nOffset);

	bRetCode = GetStopPos(pszBuffer, uSize, IsASCII, &nLineCounter, &nColCounter, &nOffset);
	if (bRetCode)
	{
		printf("ASCII stop at line:%d col:%d, offset:0x%x(%d)\n", nLineCounter, nColCounter, nOffset, nOffset);

		if (nOffset < uSize)
		{
			char szTry[4] = { pszBuffer[nOffset], pszBuffer[nOffset + 1], '\0' };
			printf("try output: '%s' (0x%04x %04x)\n", szTry, (unsigned char)pszBuffer[nOffset], (unsigned char)pszBuffer[nOffset + 1]);
		}
		else
		{
			printf("file end incomplete: 0x%04x", (unsigned char)pszBuffer[nOffset]);
		}
	}

	return;
}

bool Coder::Check()
{
	bool					bResult		= false;
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

		KG_PROCESS_ERROR(pBOMDesc == s_cpUTF8Bom);

		bUTF8Pass = CheckForCode(m_pszBuffer + pBOMDesc->nLen, m_uFileSize - pBOMDesc->nLen, IsUTF8);
		KG_PROCESS_ERROR(bUTF8Pass);
	}
	else
	{
		bASICCPass = CheckForCode(m_pszBuffer, m_uFileSize, IsASCII);
		bGBKPass = CheckForCode(m_pszBuffer, m_uFileSize, IsGBK);
		bUTF8Pass = CheckForCode(m_pszBuffer, m_uFileSize, IsUTF8);

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
			if (pBOMDesc != s_cpUTF8Bom)
			{
				printf("[ERROR] bom type is %s : %s\n", pBOMDesc->cpszName, m_strPath.c_str());
			}
			else if (!bUTF8Pass)
			{
				printf("[ERROR] have utf8 bom, but content not utf8: %s\n", m_strPath.c_str());
				ShowConfusionPos(m_pszBuffer + s_cpUTF8Bom->nLen, m_uFileSize - s_cpUTF8Bom->nLen);
			}
			else
			{
				assert(0);
			}
		}
		else
		{
			printf("[ERROR] Unrecognized: %s\n", m_strPath.c_str());
			ShowConfusionPos(m_pszBuffer, m_uFileSize);
		}
	}
	else
	{
		if (bUTF8Pass)
		{
//  		SetConsoleColor c(FOREGROUND_BLUE);
//  		printf("[UTF8] %s\n", m_strPath.c_str());
		}
		else if (bGBKPass)
		{
// 			SetConsoleColor c(FOREGROUND_GREEN);
// 			printf("[GBK] %s\n", m_strPath.c_str());
		}
		else if (m_uFileSize == 0)
		{
// 			SetConsoleColor c(FOREGROUND_GREEN);
// 			printf("[EMPTY] %s\n", m_strPath.c_str());
		}
		else
		{
			assert(0);
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
