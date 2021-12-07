#pragma once
#include <stdio.h>
#include <string>

class Coder
{
public:
	bool LoadFile(const char* pszPath);
	void Release();

	bool Check();

private:
	struct BOMDesc* GetBOMType();

private:
	char* m_pszBuffer = nullptr;
	size_t m_uFileSize = 0;
	std::string m_strPath;
};

