#pragma once

#include <stdio.h>
#include <windows.h>
#include <time.h>

#define MAX_BUFFER 1024 * 60
#define MAX_ATTR 64
#define SAVE_DAY 7

enum Log_Level
{
	Log_Info,
	Log_Opt,
	Log_Err
};

class CLog
{
public:
	~CLog();
	static CLog* instanse(const char* sDir = NULL, const char* sPrifix = NULL);
	bool addLog(Log_Level level, const char* sFormat, ...);
	static void setEncrypt(BOOL bEncrypt = TRUE);
	static void setEnable(BOOL);

private:
	CLog(const char* sDir = NULL, const char* sPrifix = NULL);
	CLog(const CLog&);
	char* curTime(char* buff, struct tm* time1 = NULL, time_t subSecond = 0);
	void openLog(const char* sDate);
	void write(char* str, int nlength);
	void autoDeleteFile();
	void mkdir();
private:
	static CLog* m_log;
	static BOOL m_bEncrypt;
	static BOOL m_bEnable;
	FILE* m_file;
	char m_sText[MAX_BUFFER];
	char m_attrDir[MAX_ATTR];
	char m_attrTempName[MAX_ATTR];
	char m_attr[MAX_ATTR];
	char m_absDir[MAX_PATH];
	char m_absFile[MAX_PATH];
	BOOL m_bIsOpen;
	CRITICAL_SECTION m_Lock;
};

#define Initlog(x) CLog::setEncrypt(x)
#define InitLogInstance(x, y) CLog::instanse(x, y)
#define LogEnable(x) CLog::setEnable(x)
#define Log(l,x,...) CLog::instanse()->addLog(l,x,__VA_ARGS__)