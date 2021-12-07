#include "stdafx.h"
#include "FileList.h"
#include <string>

bool CutPathToRelative(char szStrBuffer[], size_t uStrBufferSize, const char* cpszTarget, const char* cpszNewHeader)
{
	bool bResult = false;
	int nRetCode = 0;
	char szTmp[MAX_PATH];
	char szFormat[128];

	snprintf(szFormat, sizeof(szFormat), "%s\\%%s", cpszTarget);

	nRetCode = sscanf_s(szStrBuffer, szFormat, szTmp, (unsigned)sizeof(szTmp));
	KGLOG_PROCESS_ERROR(nRetCode == 1);

	nRetCode = snprintf(szStrBuffer, uStrBufferSize, "%s%s", cpszNewHeader, szTmp);
	KGLOG_PROCESS_ERROR(nRetCode > 0 && nRetCode < uStrBufferSize);
	szStrBuffer[uStrBufferSize - 1] = '\0';

	bResult = true;
Exit0:
	return bResult;
}

bool FileList::IsMatchFile(const char cszFileName[])
{
	bool        bResult = false;
	const char* pcExtPos = NULL;
	const char* pcPos = NULL;
	char        szExtName[8];

	pcPos = cszFileName;
	while (*pcPos)
	{
		if (*pcPos == '.')
		{
			pcExtPos = pcPos + 1;
		}
		pcPos++;
	}

	KG_PROCESS_ERROR(pcExtPos != NULL && pcPos - pcExtPos < sizeof(szExtName));

	strcpy_s(szExtName, sizeof(szExtName), pcExtPos);

	for (auto& rStrExtName : m_extNames)
	{
		if (_stricmp(szExtName, rStrExtName.c_str()) == 0)
		{
			bResult = true;
			break;
		}
	}

Exit0:
	return bResult;
}

bool FileList::_Search(const char cszDir[])
{
	BOOL            bResult = false;
	int			    nRetCode = false;
	HANDLE          hFind = INVALID_HANDLE_VALUE;
	DWORD           dwScriptID = 0;
	WIN32_FIND_DATA FindFileData;
	char            szPath[MAX_PATH];
	char            szTmp[MAX_PATH];

	nRetCode = snprintf(szPath, sizeof(szPath), "%s\\*", cszDir);
	KGLOG_PROCESS_ERROR(nRetCode > 0 && nRetCode < (int)sizeof(szPath));

	hFind = FindFirstFile(szPath, &FindFileData);
	KGLOG_PROCESS_ERROR(hFind != INVALID_HANDLE_VALUE);

	do
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (
				strcmp(FindFileData.cFileName, ".") != 0 &&
				strcmp(FindFileData.cFileName, "..") != 0 &&
				strcmp(FindFileData.cFileName, ".svn") != 0
				)
			{
				nRetCode = snprintf(szPath, sizeof(szPath), "%s\\%s", cszDir, FindFileData.cFileName);
				KGLOG_PROCESS_ERROR(nRetCode > 0 && nRetCode < (int)sizeof(szPath));

				nRetCode = _Search(szPath);
				KGLOG_PROCESS_ERROR(nRetCode);
			}
		}
		else
		{
			nRetCode = snprintf(szPath, sizeof(szPath), "%s\\%s", cszDir, FindFileData.cFileName);
			KGLOG_PROCESS_ERROR(nRetCode > 0 && nRetCode < (int)sizeof(szPath));

			nRetCode = IsMatchFile(szPath);
			
			if (nRetCode)
			{
				m_FileTable.push_back(szPath);
			}
		}
	} while (FindNextFile(hFind, &FindFileData));

	bResult = true;
Exit0:
	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}
	return bResult;
}

bool FileList::SearchFileList(const char cszDir[])
{
	assert(cszDir);

	strcpy_s(m_szBaseDir, sizeof(m_szBaseDir), cszDir);
	m_szBaseDir[sizeof(m_szBaseDir) - 1] = '\0';

	return _Search(cszDir);
}

bool FileList::AddExtNameFilter(const std::string& rExtName)
{
	if (rExtName.empty())
		return false;
	if (rExtName[0] == '.')
		m_extNames.push_back(rExtName.substr(1));
	else
		m_extNames.push_back(rExtName);
	return true;
}
