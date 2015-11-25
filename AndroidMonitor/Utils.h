#pragma once

#include <Windows.h>

#define BUF_LEN (10240)
#undef min

#define LOG_VAL 0
#if LOG_VAL == 0
#define LOG /##/
#else
#define LOG Utils::log
#endif
//#define LOG (LOG_VAL == 0) ? 0 : Utils::log

class Utils
{
public:
	Utils(void);
	~Utils(void);
	static void init(HWND edit, HWND dlg, LPTSTR title);
	static void err(LPCTSTR tag, LPCTSTR msg, DWORD errcode);
	static void err(LPCTSTR tag, LPCTSTR fmt, ...);
	static void log(LPCTSTR tag, LPCTSTR fmt, ...);
	static void alertErr(LPCTSTR msg);
	static void alertInfo(LPCTSTR msg);
	static int  min(int a, int b);
	static bool isOkay(const char* buf);
	static bool isFail(const char* buf);
	static void saveBitmap(LPBITMAPINFO bmpInfo, char* bmpData, DWORD bmpDataSize);

private:
	static CRITICAL_SECTION csLog;
	static HWND logCtrl;
	static HWND mainDlg;
	static LPTSTR alertTitle;
	static TCHAR buf[BUF_LEN];
	static TCHAR logFile[MAX_PATH];

	static void dumpLog(BOOL append=TRUE);
};
