#include "stdafx.h"
#include "DebugInfoPrint.h"

/*
调试信息打印，使用debugview查看
*/
VOID OutputDebugPrintf(const _TCHAR* strOutputString, ...)
{
	TCHAR strBuffer[OUTPUT_DEBUG_BUF_LEN] = { 0 };
	va_list vlArgs;
	va_start(vlArgs, strOutputString);
	_vstprintf_s(strBuffer, sizeof(strBuffer) / sizeof(*strBuffer), strOutputString, vlArgs);
	va_end(vlArgs);
	OutputDebugString(strBuffer);
}