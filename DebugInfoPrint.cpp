#include "stdafx.h"
#include "DebugInfoPrint.h"

/*
������Ϣ��ӡ��ʹ��debugview�鿴
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