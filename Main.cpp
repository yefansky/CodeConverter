#include "stdafx.h"
#include <string>
#include "FileList.h"
#include "Coder.h"
#include <locale.h>

int main(int nArgc, char* pArgv[])
{
	bool bRetCode = false;
	FileList fileList;
	Coder coder;
	std::vector<std::string> searchPaths;
	const char* szExtNames[] = { ".h", ".c", ".cpp", ".hpp" };
	const char* szIgnoreDirs[] = { "DevEnv"};

	setlocale(LC_ALL, "");

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

	for (auto szIgnore : szIgnoreDirs)
		fileList.AddIgnoreDirFilter(szIgnore);

	for (auto& rStrPath : searchPaths)
		fileList.SearchFileList(rStrPath.c_str());

	coder.m_bOutputAllFileType	= false;	// 关闭 输出所有可识别文件现在的编码格式
	coder.m_bOutputBOMMissMatch = false;	// 有BOM的就不再做检查了
	coder.m_bOutputUnrecognize	= true;		// 输出所有不可识别的文件清单	

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

