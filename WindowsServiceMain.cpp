#include "stdafx.h"
#include "DebugInfoPrint.h"
#include "ServiceCtrl.h"
#include "WebPrintWS.h"
#include "GlobalVariables.h"

//#ifdef  UNICODE   
//#define _tslen     wcslen  
//#else  
//#define _tslen     strlen  
//#endif 

/** Window Service **/
VOID ServiceMainProc();
VOID WorkerProcess();
VOID WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
VOID WINAPI ServiceHandler(DWORD fdwControl);
VOID MySetServiceStatus();

/** Window Service **/
const int nBufferSize = 500;
//windows��������
TCHAR pServiceName[nBufferSize + 1];
//windows�������·��
TCHAR pExeFile[nBufferSize + 1];
//�����в���
TCHAR lpCmdLineData[nBufferSize + 1];
//�����Ƿ�����
BOOL ProcessStarted = TRUE;

#define SERVICE_NAME			_T("TopWebPrintService")
CRITICAL_SECTION		myCS;
SERVICE_TABLE_ENTRY		ServiceTable[] =
{
	{pServiceName, ServiceMain},
	{NULL, NULL}
};

/**
_tmain()��΢�����ϵͳ��windows���ṩ�Ķ�unicode�ַ�����ANSI�ַ��������Զ�ת���õĳ�����ڵ㺯����
����ǩ��Ϊ:
int _tmain(int argc, TCHAR *argv[])
�������ǰ���ַ���Ϊunicodeʱ��int _tmain(int argc, TCHAR *argv[])�ᱻ�����
int wmain(int argc, wchar_t *argv[])
�������ǰ���ַ���ΪANSIʱ��int _tmain(int argc, TCHAR *argv[])�ᱻ�����
int main(int argc, char *argv[])
*/
int _tmain(int argc, _TCHAR* argv[])
{
	OutputDebugPrintf(_T("[SanpoWebPrintWS] _tmain: Entry"));
	if (argc >= 2) {
		_tcscpy_s(lpCmdLineData, _tcslen(argv[1]) + 1, argv[1]);
	}
	ServiceMainProc();
	OutputDebugPrintf(_T("[SanpoWebPrintWS] _tmain: Exit"));
	return 0;
}

/*
Windows���������
*/
VOID ServiceMainProc()
{
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMainProc: Entry"));
	//�����̵ĸ����߳̿���ʹ���ٽ���Դ���������ͬ���������⣬�ö����ܱ�֤�ĸ��߳��ܹ���õ��ٽ���Դ���󣬸�ϵͳ�ܹ�ƽ�ĶԴ�ÿһ���̡߳�
	::InitializeCriticalSection(&myCS);
	TCHAR pModuleFile[nBufferSize + 1];
	DWORD dwSize = GetModuleFileName(NULL, pModuleFile, nBufferSize);
	pModuleFile[dwSize] = 0;
	if (dwSize > 4 && pModuleFile[dwSize - 4] == '.')
	{
		_stprintf_s(pExeFile, _T("%s"), pModuleFile);
	}
	_tcscpy_s(pServiceName, SERVICE_NAME);
	if (_tcsicmp(_T("-s"), lpCmdLineData) == 0) {
		//��������
		OutputDebugPrintf(_T("[SanpoWebPrintWS] Proc: RunService"));
		RunService(pServiceName);
	}
	else if (_tcsicmp(_T("-k"), lpCmdLineData) == 0) {
		//ֹͣ����
		OutputDebugPrintf(_T("[SanpoWebPrintWS] Proc: KillService"));
		KillService(pServiceName);
	}
	else if (_tcsicmp(_T("-i"), lpCmdLineData) == 0) {
		//��װ����
		OutputDebugPrintf(_T("[SanpoWebPrintWS] Proc: Install"));
		Install(pExeFile, pServiceName);
	}
	else if (_tcsicmp(_T("-u"), lpCmdLineData) == 0) {
		//ж�ط���
		OutputDebugPrintf(_T("[SanpoWebPrintWS] Proc: UnInstall"));
		UnInstall(pServiceName);
	}
	else {
		//
		OutputDebugPrintf(_T("[SanpoWebPrintWS] Proc: Other"));
		WorkerProcess();
	}
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMainProc: Exit"));
}

VOID WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMain: Entry"));

	g_StatusHandle = RegisterServiceCtrlHandler(pServiceName, ServiceHandler);
	if (g_StatusHandle == 0)
	{
		long nError = GetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] RegisterServiceCtrlHandler failed, error code = %d"), nError);
		return;
	}

	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	//g_ServiceStatus.dwServiceType = SERVICE_WIN32;
	//ָ���������ʲô���Ŀ���֪ͨ��
	//�������һ��������Ƴ���SCP��ȥ��ͣ / �������񣬾Ͱ������SERVICE_ACCEPT_PAUSE_CONTINUE��
	//�ܶ����֧����ͣ��������ͱ����Լ������ڷ��������Ƿ���á�
	//���������һ��SCPȥֹͣ���񣬾�Ҫ������ΪSERVICE_ACCEPT_STOP��
	//�������Ҫ�ڲ���ϵͳ�رյ�ʱ��õ�֪ͨ��������ΪSERVICE_ACCEPT_SHUTDOWN�����յ�Ԥ�ڵĽ����
	//��Щ��־�����á� | ���������ϡ�
	g_ServiceStatus.dwControlsAccepted = 0;
	//g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	//g_ServiceStatus.dwWaitHint = 0;
	MySetServiceStatus();

	//ִ�������������������
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMain: Performing Service Start Operations"));
	// ����ֹͣ�¼�����
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL)
	{
		long nError = GetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMain: CreateEvent(g_ServiceStopEvent) failed, error code = %d"), nError);

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_ACCEPT_STOP;
		g_ServiceStatus.dwWin32ExitCode = nError;
		g_ServiceStatus.dwCheckPoint = 1;
		MySetServiceStatus();
		return;
	}
	//���߷��������������������
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	MySetServiceStatus();

	//������ִ�з�����Ҫ������߳�
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMain: Waiting for Worker Thread to complete"));

	// �ȴ���ֱ�����ǵĹ����߳���Ч�˳��������÷�����Ҫֹͣ
	if (0 != hThread) {
		WaitForSingleObject(hThread, INFINITE);
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMain: Worker Thread Stop Event signaled"));
	}


	//ִ����������
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMain: Performing Cleanup Operations"));
	CloseHandle(g_ServiceStopEvent);

	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;
	MySetServiceStatus();
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMain: Exit"));
}

VOID WINAPI ServiceHandler(DWORD fdwControl)
{
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceCtrlHandler: Entry"));
	switch (fdwControl)
	{
	case SERVICE_CONTROL_STOP:
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceCtrlHandler: SERVICE_CONTROL_STOP Request"));
	case SERVICE_CONTROL_SHUTDOWN:
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceCtrlHandler: SERVICE_CONTROL_SHUTDOWN Request"));
		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
			break;
		}
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;
		MySetServiceStatus();
		SetEvent(g_ServiceStopEvent);
		break;
	case SERVICE_CONTROL_PAUSE:
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceCtrlHandler: SERVICE_PAUSED Request"));
		g_ServiceStatus.dwCurrentState = SERVICE_PAUSED;
		break;
	case SERVICE_CONTROL_CONTINUE:
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceCtrlHandler: SERVICE_CONTROL_CONTINUE Request"));
		g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
		break;
	case SERVICE_CONTROL_INTERROGATE:
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceCtrlHandler: SERVICE_CONTROL_INTERROGATE Request"));
		break;
	default:
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceCtrlHandler: %d Request"), fdwControl);
		break;
	};
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceCtrlHandler: Exit"));
	return;
}

/*
��������̵����߳����ӵ�������ƹ���������ʹ���̳߳�Ϊ���ý��̵ķ�����Ƶ��ȳ����߳�
*/
VOID WorkerProcess() {
	OutputDebugPrintf(_T("[SanpoWebPrintWS] WorkerProcess: Entry"));
	if (!StartServiceCtrlDispatcher(ServiceTable))
	{
		long nError = GetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] StartServiceCtrlDispatcher failed, error code = %d"), nError);
	}
	::DeleteCriticalSection(&myCS);
	OutputDebugPrintf(_T("[SanpoWebPrintWS] WorkerProcess: Exit"));
}


VOID MySetServiceStatus() {
	if (!SetServiceStatus(g_StatusHandle, &g_ServiceStatus))
	{
		long nError = GetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceCtrlHandler: SetServiceStatus failed, error code = %d"), nError);
	}
}



