#include "stdafx.h"
#include <string>
#include "FileList.h"
#include "Coder.h"

int main(int nArgc, char* pArgv[])
{
	bool bRetCode = false;
	FileList fileList;
	Coder coder;

	std::string strSearchPath = ".";
	const char* szExtNames[] = { ".h", ".c", ".cpp", ".hpp" };

	for (int i = 1; i < nArgc; i++)
	{
		strSearchPath = pArgv[i];
	}

	for (auto szExtName : szExtNames)
		fileList.AddExtNameFilter(szExtName);

	fileList.SearchFileList(strSearchPath.c_str());

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

