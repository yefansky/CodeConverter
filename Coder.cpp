#include "stdafx.h"
#include "Coder.h"

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

int IsASICII(char* ch)
{
	assert(ch);
	if (*ch >= 0 && *ch <= 0x7F)
		return 1;
	return 0;
}

int IsGBK(char* ch, size_t uLeftSize)
{
	if (IsASICII(ch))
		return 1;
	if (uLeftSize < 2)
		return 0;
	unsigned char ch1 = *ch;
	unsigned char ch2 = *(ch + 1);
	if ((ch1 >= 0xA1 && ch1 <= 0xA9 && ch2 >= 0xA1 && ch2 <= 0xFE) ||
		(ch1 >= 0xB0 && ch1 <= 0xF7 && ch2 >= 0xA1 && ch2 <= 0xFE) ||
		(ch1 >= 0x81 && ch1 <= 0xA0 && ch2 >= 0x40 && ch2 <= 0xFE && ch2 != 0x7F) ||
		(ch1 >= 0xAA && ch1 <= 0xFE && ch2 >= 0x40 && ch2 <= 0xA0 && ch2 != 0x7F) ||
		(ch1 >= 0xA8 && ch1 <= 0xA9 && ch2 >= 0x40 && ch2 <= 0xA0 && ch2 != 0x7F) ||
		(ch1 >= 0xAA && ch1 <= 0xAF && ch2 >= 0xA1 && ch2 <= 0xFE) ||
		(ch1 >= 0xF8 && ch1 <= 0xFE && ch2 >= 0xA1 && ch2 <= 0xFE) ||
		(ch1 >= 0xA1 && ch1 <= 0xA7 && ch2 >= 0x40 && ch2 <= 0xA0 && ch2 != 0x7F))
		return 2;
	return 0;
}

int IsUTF8(char* ch, size_t uLeftSize)
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
		return IsASICII(ch);

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

bool Coder::Check()
{
	bool bResult = false;
	size_t uLeftSize = m_uFileSize;
	bool bASICCPass = true;
	bool bGBKPass = true;
	bool bUTF8Pass = true;
	char* pszOffset = nullptr;

	assert(m_uFileSize >= 0);

	KG_PROCESS_SUCCESS(m_uFileSize == 0);
	assert(m_pszBuffer);

	pszOffset = m_pszBuffer;
	uLeftSize = m_uFileSize;

	if (HaveBom())
	{
		bUTF8Pass = true;
	}
	else
	{
		while (uLeftSize)
		{
			if (IsASICII(pszOffset) == 0)
			{
				bASICCPass = false;
				break;
			}
			pszOffset++;
			uLeftSize--;
		}

		pszOffset = m_pszBuffer;
		uLeftSize = m_uFileSize;
		while (uLeftSize)
		{
			int nLen = IsGBK(pszOffset, uLeftSize);
			if (nLen == 0)
			{
				bGBKPass = false;
				break;
			}
			pszOffset += nLen;
			uLeftSize -= nLen;
		}

		pszOffset = m_pszBuffer;
		uLeftSize = m_uFileSize;
		while (uLeftSize)
		{
			int nLen = IsUTF8(pszOffset, uLeftSize);
			if (nLen == 0)
			{
				bUTF8Pass = false;
				break;
			}
			pszOffset += nLen;
			uLeftSize -= nLen;
		}

		KG_PROCESS_ERROR((bGBKPass || bUTF8Pass) && !(bGBKPass && bUTF8Pass && !bASICCPass));
	}

Exit1:
	bResult = true;
Exit0:
	if (!bResult)
	{
		SetConsoleColor c(FOREGROUND_RED);
		printf("[ERROR] %s\n", m_strPath.c_str());
	}
	else
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
	return bResult;
}

bool Coder::HaveBom()
{
	bool bResult = false;
	const char szUTF8Bom[] = { (char)0xEF, (char)0xBB, (char)0xBF};
	int i = 0;

	assert(m_pszBuffer && m_uFileSize > 0);

	KG_PROCESS_ERROR(m_uFileSize >= 3);
	
	for (auto ch : szUTF8Bom)
	{
		KG_PROCESS_ERROR(m_pszBuffer[i++] == ch);
	}

	bResult = true;
Exit0:
	return bResult;
}
