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
//windows服务名称
TCHAR pServiceName[nBufferSize + 1];
//windows服务程序路径
TCHAR pExeFile[nBufferSize + 1];
//命令行参数
TCHAR lpCmdLineData[nBufferSize + 1];
//服务是否启动
BOOL ProcessStarted = TRUE;

#define SERVICE_NAME			_T("TopWebPrintService")
CRITICAL_SECTION		myCS;
SERVICE_TABLE_ENTRY		ServiceTable[] =
{
	{pServiceName, ServiceMain},
	{NULL, NULL}
};

/**
_tmain()是微软操作系统（windows）提供的对unicode字符集和ANSI字符集进行自动转换用的程序入口点函数。
函数签名为:
int _tmain(int argc, TCHAR *argv[])
当你程序当前的字符集为unicode时，int _tmain(int argc, TCHAR *argv[])会被翻译成
int wmain(int argc, wchar_t *argv[])
当你程序当前的字符集为ANSI时，int _tmain(int argc, TCHAR *argv[])会被翻译成
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
Windows服务处理入口
*/
VOID ServiceMainProc()
{
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMainProc: Entry"));
	//单进程的各个线程可以使用临界资源对象来解决同步互斥问题，该对象不能保证哪个线程能够获得到临界资源对象，该系统能公平的对待每一个线程。
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
		//启动服务
		OutputDebugPrintf(_T("[SanpoWebPrintWS] Proc: RunService"));
		RunService(pServiceName);
	}
	else if (_tcsicmp(_T("-k"), lpCmdLineData) == 0) {
		//停止服务
		OutputDebugPrintf(_T("[SanpoWebPrintWS] Proc: KillService"));
		KillService(pServiceName);
	}
	else if (_tcsicmp(_T("-i"), lpCmdLineData) == 0) {
		//安装服务
		OutputDebugPrintf(_T("[SanpoWebPrintWS] Proc: Install"));
		Install(pExeFile, pServiceName);
	}
	else if (_tcsicmp(_T("-u"), lpCmdLineData) == 0) {
		//卸载服务
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
	//指明服务接受什么样的控制通知。
	//如果允许一个服务控制程序（SCP）去暂停 / 继续服务，就把它设成SERVICE_ACCEPT_PAUSE_CONTINUE。
	//很多服务不支持暂停或继续，就必须自己决定在服务中它是否可用。
	//如果你允许一个SCP去停止服务，就要设置它为SERVICE_ACCEPT_STOP。
	//如果服务要在操作系统关闭的时候得到通知，设置它为SERVICE_ACCEPT_SHUTDOWN可以收到预期的结果。
	//这些标志可以用“ | ”运算符组合。
	g_ServiceStatus.dwControlsAccepted = 0;
	//g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	//g_ServiceStatus.dwWaitHint = 0;
	MySetServiceStatus();

	//执行启动服务所需的任务
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMain: Performing Service Start Operations"));
	// 创建停止事件对象
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
	//告诉服务控制器我们正在启动
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	MySetServiceStatus();

	//启动将执行服务主要任务的线程
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMain: Waiting for Worker Thread to complete"));

	// 等待，直到我们的工作线程有效退出，表明该服务需要停止
	if (0 != hThread) {
		WaitForSingleObject(hThread, INFINITE);
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceMain: Worker Thread Stop Event signaled"));
	}


	//执行清理任务
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
将服务进程的主线程连接到服务控制管理器，这使得线程成为调用进程的服务控制调度程序线程
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



