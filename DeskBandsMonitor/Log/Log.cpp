#include "Log.h"
#pragma warning(disable: 4996)
#include <io.h>

CLog* CLog::m_log = NULL;
BOOL CLog::m_bEncrypt = FALSE;
BOOL CLog::m_bEnable = FALSE;

CLog::CLog(const char* sDir, const char* sPrifix)
{
	if (!m_bEnable)
		return;
	m_file = NULL;
	m_bIsOpen = FALSE;
	InitializeCriticalSection(&m_Lock);

	memset(m_absDir, 0x00, sizeof(m_absDir));
	if (!sDir || !strlen(sDir))
	{
		::GetModuleFileNameA(NULL, m_absDir, MAX_PATH);
		char *pos = strrchr(m_absDir, '\\');
		if (pos) *pos = 0;
	}
	else {
		int n = sprintf(m_absDir, sDir);
		m_absDir[n] = 0;
	}

	int n = sprintf(m_attr, ".txt");
	m_attr[n] = 0;
	n = sprintf(m_attrDir, "Log");
	m_attrDir[n] = 0;

	mkdir();

	if (!sPrifix)
		n = sprintf(m_attrTempName, "Log_");
	else
		n = sprintf(m_attrTempName, sPrifix);
	m_attrTempName[n] = 0;
	char timebuff[32] = {};
	openLog(curTime(timebuff));
}

CLog::CLog(const CLog&)
{
}

CLog::~CLog()
{
	Log(Log_Info, "log quit");
	DeleteCriticalSection(&m_Lock);
	if (m_file) fclose(m_file);
}

void CLog::mkdir()
{
	char logDir[1024] = {};
	memset(logDir, 0x00, sizeof(logDir));
	sprintf(logDir, "%s\\%s", m_absDir, m_attrDir);

	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;

	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = &sd;

	if (!CreateDirectoryA(logDir, &sa))
	{
		char buff[1024] = {};
		sprintf_s(buff, 1024, "[Log] create dir[%s] failed, %d\n", logDir, GetLastError());
		OutputDebugStringA(buff);

		if (GetLastError() == 5)
		{
			char buffer[MAX_PATH] = { 0 };
			if (GetTempPathA(MAX_PATH, buffer) == 0)
			{
				char buff[1024] = {};
				sprintf_s(buff, 1024, "[Log] get temp dir failed, %d\n", GetLastError());
				OutputDebugStringA(buff);
			}
			else
			{
				char *pos = strrchr(buffer, '\\');
				if (pos) *pos = 0;

				memset(m_absDir, 0x00, sizeof(m_absDir));
				sprintf_s(m_absDir, MAX_PATH, buffer);

				memset(logDir, 0x00, sizeof(logDir));
				sprintf(logDir, "%s\\%s", m_absDir, m_attrDir);
				if (!CreateDirectoryA(logDir, &sa))
				{
					char buff[1024] = {};
					sprintf_s(buff, 1024, "[Log] create dir[%s] failed, %d\n", logDir, GetLastError());
					OutputDebugStringA(buff);
				}
			}
		}
	}

}
CLog* CLog::instanse(const char* sDir, const char* sPrifix)
{
	if (!m_log)
		m_log = new CLog(sDir, sPrifix);

	return m_log;
}

bool CLog::addLog(Log_Level level, const char* sFormat, ...)
{
	if (!m_bEnable) return true;
	if (!m_bIsOpen) return true;
	EnterCriticalSection(&m_Lock);
	struct tm tm1;
	char sDate[32] = {};
	bool bret = true;
	if (!strstr(m_absFile, curTime(sDate, &tm1)) || !m_file)
	{
		bret = false;
		openLog(sDate);
	}
	memset(m_sText, 0x00, sizeof(m_sText));
	int n = 0;
	switch (level)
	{
	case Log_Err:
		n += sprintf(m_sText, "%-6s", "[ERR]");
		break;
	case Log_Opt:
		n += sprintf(m_sText, "%-6s", "[OPT]");
		break;
	default:
		n += sprintf(m_sText, "%-6s", "[INFO]");
		break;
	}

	n += sprintf(m_sText + n, " %04d-%02d-%02d %02d:%02d:%02d ", tm1.tm_year + 1900, 1 + tm1.tm_mon, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);
	char* pNext = m_sText + n;
	va_list args;
	va_start(args, sFormat);
	vsprintf_s(pNext, sizeof(m_sText) - n, sFormat, args);
	va_end(args);

	write(m_sText, (int)strlen(m_sText));
	LeaveCriticalSection(&m_Lock);

	return bret;
}

void CLog::setEncrypt(BOOL bEncrypt /*= TRUE*/)
{
	m_bEncrypt = bEncrypt;
}

void CLog::setEnable(BOOL bEnable)
{
	m_bEnable = bEnable;
}

char* CLog::curTime(char* buff, struct tm* time1, time_t subSecond)
{
	time_t t = time(NULL);
	t -= subSecond;
	struct tm tm1;
#ifdef WIN32  
	localtime_s(&tm1, &t);
#else  
	localtime_r(&t, &tm1);
#endif 
	if (time1) *time1 = tm1;
	int n = sprintf(buff, "%04d-%02d-%02d", tm1.tm_year + 1900, 1 + tm1.tm_mon, tm1.tm_mday);
	buff[n] = 0;
	return buff;
}

void CLog::openLog(const char* sDate)
{
	if (m_file)	fclose(m_file);
	memset(m_absFile, 0x00, sizeof(m_absFile));
	sprintf(m_absFile, "%s\\%s\\%s%s%s", m_absDir, m_attrDir, m_attrTempName, sDate, m_attr);
	
	m_file = fopen(m_absFile, "a+");
	m_bIsOpen = m_file != NULL;
	if (!m_bIsOpen)
	{
		char buff[1024] = {};
		sprintf_s(buff, 1024, "[Log] open file[%s] failed, %d\n", m_absFile, GetLastError());
		OutputDebugStringA(buff);
		return;
	}
	char buff[100] = "===============================Log=================================";
	write(buff, (int)strlen(buff));
	autoDeleteFile();
}

void CLog::write(char* str, int nlength)
{
	if (!m_file) return;
	strcat(str, "\n");
	fwrite(str, nlength + 1, 1, m_file);
	fflush(m_file);
}

void CLog::autoDeleteFile()
{
	char findFile[MAX_PATH] = {};
	sprintf(findFile, "%s\\%s\\*%s", m_absDir, m_attrDir, m_attr);
	struct tm tm1;
	char sDate[32] = {};
	curTime(sDate, &tm1, SAVE_DAY * 24 * 3600);
	strcat(sDate, m_attr);
	auto attrlen = strlen(m_attrTempName);
	WIN32_FIND_DATAA finder;
	HANDLE hFind = FindFirstFileA(findFile, &finder);
	if (hFind == INVALID_HANDLE_VALUE) return;
	do
	{
		if ((finder.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) == FILE_ATTRIBUTE_ARCHIVE)
		{
			auto pDest = strstr(finder.cFileName, m_attrTempName);
			if (pDest)
			{
				pDest += attrlen;
				if (strcmp(sDate, pDest) >= 0)
				{
					char delfile[MAX_PATH] = {};
					sprintf(delfile, "%s\\%s\\%s", m_absDir, m_attrDir, finder.cFileName);
					DeleteFileA(delfile);
				}
			}
		}
	} while (FindNextFileA(hFind, &finder));

	FindClose(hFind);
}