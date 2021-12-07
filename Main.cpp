#include "stdafx.h"
#include <string>
#include "FileList.h"
#include "Coder.h"

int main(int nArgc, char* pArgv[])
{
	bool bRetCode = false;
	FileList fileList;
	Coder coder;
	std::vector<std::string> searchPaths;
	const char* szExtNames[] = { ".h", ".c", ".cpp", ".hpp" };

	if (nArgc <= 1)
	{
		searchPaths.push_back(".");
	}
	else
	{
		for (int i = 1; i < nArgc; i++)
		{
			searchPaths.push_back(pArgv[i]);
		}
	}

	for (auto szExtName : szExtNames)
		fileList.AddExtNameFilter(szExtName);

	for (auto& rStrPath : searchPaths)
		fileList.SearchFileList(rStrPath.c_str());

	for (auto& rStrPath : fileList.GetList())
	{
		bRetCode = coder.LoadFile(rStrPath.c_str());
		KGLOG_PROCESS_ERROR(bRetCode);

		bRetCode = coder.Check();
		//KGLOG_PROCESS_ERROR(bRetCode);

		coder.Release();
	}

Exit0:
	return 0;
}

