#pragma once
#include <string>
#include <vector>
#include <windows.h>

typedef std::vector<std::string> FILE_TABLE;

class FileList
{
private:
	char m_szBaseDir[MAX_PATH];

	FILE_TABLE  m_FileTable;
	std::vector<std::string> m_extNames;

public:
	bool SearchFileList(const char cszDir[]);
	bool AddExtNameFilter(const std::string& rExtName);
	const FILE_TABLE& GetList()
	{
		return m_FileTable;
	}

private:
	bool IsMatchFile(const char cszFileName[]);
	bool _Search(const char cszDir[]);
};

