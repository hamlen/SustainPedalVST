#include "log.h"

#ifdef LOGGING

#include "Windows.h"
#include <ShlObj.h>
#include <stdio.h>

WCHAR logfilename[MAX_PATH] = {};

static void init_log()
{
	WCHAR* buf;
	if (SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, NULL, &buf) == S_OK)
	{
		wcscpy_s(logfilename, buf);
		wcscat_s(logfilename, L"\\SustainPedalVST.log");
	}
	CoTaskMemFree(buf);
}

void log(const char* format, ...)
{
	char buf[2048];
	va_list args;
	va_start(args, format);
	vsprintf_s(buf, sizeof(buf) / sizeof(*buf), format, args);
	va_end(args);

	if (!*logfilename) init_log();
	if (!*logfilename) return;
	HANDLE h = CreateFile(logfilename, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h != INVALID_HANDLE_VALUE)
	{
		SYSTEMTIME time;
		char datebuf[128];
		GetLocalTime(&time);
		sprintf_s(datebuf, sizeof(datebuf) / sizeof(*datebuf), "%04d/%02d/%02d %02d:%02d:%02d.%03d  ", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
		WriteFile(h, datebuf, (DWORD)strlen(datebuf), NULL, NULL);
		WriteFile(h, buf, (DWORD)strlen(buf), NULL, NULL);
		CloseHandle(h);
	}
}

#endif
