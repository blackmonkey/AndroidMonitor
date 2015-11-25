#include "stdafx.h"
#include "Utils.h"
#include <StrSafe.h>
#include <WindowsX.h>

#define DUMP_LOG

CRITICAL_SECTION Utils::csLog;
HWND Utils::logCtrl = NULL;
HWND Utils::mainDlg = NULL;
LPTSTR Utils::alertTitle = NULL;
TCHAR Utils::buf[BUF_LEN] = {0};
TCHAR Utils::logFile[MAX_PATH] = {0};

Utils::Utils(void)
{
	InitializeCriticalSection(&csLog);
}

Utils::~Utils(void)
{
	DeleteCriticalSection(&csLog);
}

void Utils::init(HWND hCtrl, HWND dlg, LPTSTR title)
{
	logCtrl = hCtrl;
	mainDlg = dlg;
	alertTitle = title;

	int len = GetModuleFileName(NULL, logFile, MAX_PATH);
	while (logFile[--len] != '\\') {}
	StringCchCopy(&logFile[++len], MAX_PATH - len, _T("log.txt"));

	// get current time.
	SYSTEMTIME lt;
	GetLocalTime(&lt);

	EnterCriticalSection(&csLog);
	StringCchPrintf(buf, BUF_LEN, _T("%04d-%02d-%02d %02d:%02d:%02d\n"),
		lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);

	// clear previous log and dump current time.
	dumpLog(FALSE);
	LeaveCriticalSection(&csLog);
}

void Utils::err(LPCTSTR tag, LPCTSTR msg, DWORD errcode)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf, 0, NULL);

	EnterCriticalSection(&csLog);
	StringCchPrintf(buf, BUF_LEN, _T("[%s]%s: %d %s"), tag, msg, errcode, lpMsgBuf);
	dumpLog();
	LeaveCriticalSection(&csLog);

	LocalFree(lpMsgBuf);
}

void Utils::err(LPCTSTR tag, LPCTSTR fmt, ...)
{
	va_list args;
	size_t tagLen = 0;

	EnterCriticalSection(&csLog);
	StringCchPrintf(buf, BUF_LEN, _T("[%s]ERROR:"), tag);
	StringCchLength(buf, BUF_LEN, &tagLen);

	va_start(args, fmt);
	StringCchVPrintf(&buf[tagLen], BUF_LEN - tagLen, fmt, args);
	va_end(args);

	dumpLog();
	LeaveCriticalSection(&csLog);
}

void Utils::alertErr(LPCTSTR msg)
{
	MessageBox(mainDlg, msg, alertTitle, MB_OK | MB_ICONERROR);
}

void Utils::alertInfo(LPCTSTR msg)
{
	MessageBox(mainDlg, msg, alertTitle, MB_OK | MB_ICONINFORMATION);
}

void Utils::log(LPCTSTR tag, LPCTSTR fmt, ...)
{
#ifdef DUMP_LOG
	va_list args;
	size_t bufLen = 0;

	EnterCriticalSection(&csLog);
	StringCchPrintf(buf, BUF_LEN, _T("[%s]"), tag);
	StringCchLength(buf, BUF_LEN, &bufLen);

	va_start(args, fmt);
	StringCchVPrintf(&buf[bufLen], BUF_LEN - bufLen, fmt, args);
	va_end(args);

	dumpLog();
	LeaveCriticalSection(&csLog);
#endif
}

void Utils::dumpLog(BOOL append)
{
	// ensure line ends with LF
	size_t bufLen = 0;
	StringCchLength(buf, BUF_LEN, &bufLen);
	for (int i = bufLen - 1; i >= 0; i--) {
		if (buf[i] == '\r') {
			buf[i] = '\n';
		} else if (buf[i] != '\n') {
			buf[++i] = '\n';
			buf[++i] = 0;
			break;
		}
	}

	if (logCtrl) {
		int txtLen = Edit_GetTextLength(logCtrl); // Obtain current text length in the window.
		Edit_SetSel(logCtrl, txtLen, txtLen);
		Edit_ReplaceSel(logCtrl, buf); // Write the new text.
	}

	HANDLE hFile = CreateFile(logFile,
							  append ? FILE_APPEND_DATA : GENERIC_WRITE,
							  FILE_SHARE_READ | FILE_SHARE_WRITE,
							  NULL,
							  append ? OPEN_ALWAYS : CREATE_ALWAYS,
							  FILE_ATTRIBUTE_NORMAL,
							  NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		size_t bytes = 0;
		DWORD bytesWritten = 0;
		StringCbLength(buf, BUF_LEN, &bytes);
		WriteFile(hFile, buf, bytes, &bytesWritten, NULL);
		CloseHandle(hFile);
	}
}

int Utils::min(int a, int b)
{
	return a < b ? a : b;
}

bool Utils::isOkay(const char* buf)
{
	return buf[0] == 'O' && buf[1] == 'K' && buf[2] == 'A' && buf[3] == 'Y';
}

bool Utils::isFail(const char* buf)
{
	return buf[0] == 'F' && buf[1] == 'A' && buf[2] == 'I' && buf[3] == 'L';
}

void Utils::saveBitmap(LPBITMAPINFO bmpInfo, char* bmpData, DWORD bmpDataSize)
{
	HANDLE hf;                 // file handle
	BITMAPFILEHEADER hdr = {0}; // bitmap file-header
	DWORD dwTmp;
	char fname[MAX_PATH];
	static int fid = 0;

	sprintf(fname, ".\\screen_%08d.bmp", fid++);
	// Create the .BMP file.
	hf = CreateFile(fname, GENERIC_READ | GENERIC_WRITE,
		(DWORD) 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);
	if (hf == INVALID_HANDLE_VALUE) return;

	hdr.bfType = 0x4d42; // 0x42 = "B" 0x4d = "M"

	// Compute the offset to the array of color indices.  
	hdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 2;

	// Compute the size of the entire file.
	hdr.bfSize = hdr.bfOffBits + bmpDataSize;

	// Copy the BITMAPFILEHEADER into the .BMP file.
	WriteFile(hf, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER), (LPDWORD)&dwTmp, NULL);

	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file.
	WriteFile(hf, bmpInfo, sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 2, (LPDWORD)&dwTmp, NULL);

	// Copy the array of color indices into the .BMP file.
	WriteFile(hf, bmpData, bmpDataSize, (LPDWORD)&dwTmp, NULL);

	// Close the .BMP file.
	CloseHandle(hf);
}
