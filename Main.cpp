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

	coder.m_bOutputAllFileType	= false;	// �ر� ������п�ʶ���ļ����ڵı����ʽ
	coder.m_bOutputBOMMissMatch = false;	// ��BOM�ľͲ����������
	coder.m_bOutputUnrecognize	= true;		// ������в���ʶ����ļ��嵥	

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

