#pragma once
#include <stdio.h>
#include <string>

class Coder
{
public:
	Coder() {}
	~Coder()
	{
		if (m_pfFile)
		{
			fclose(m_pfFile);
			m_pfFile = nullptr;
		}
	}

	bool LoadFile(const char* pszPath);
	void Release();

	bool Check();

private:
	struct BOMDesc* GetBOMType();
	void TryOutputUTF8(int nOffset);
	void TryOutputGBK(int nOffset);
	void ShowConfusionPos(const char* pszBuffer, size_t uSize, size_t uBOMLen);
	void ShowBinary(const char* pszBuffer, size_t uLen);
	void ShowHex(const char* pszBuffer, size_t uLen);
private:
	char* m_pszBuffer = nullptr;
	size_t m_uFileSize = 0;
	std::string m_strPath;
	FILE* m_pfFile = nullptr;

public:
	// output switch
	bool m_bOutputBOMMissMatch	= true;
	bool m_bOutputAllFileType	= true;
	bool m_bOutputUnrecognize	= true;
	bool m_bOutputConvertScript = false;
};

