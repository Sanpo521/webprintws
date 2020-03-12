#include "stdafx.h"
#include "ServiceCtrl.h"
#include "DebugInfoPrint.h"

/*
启动Windows服务
*/
BOOL RunService(TCHAR* pName) {
	OutputDebugPrintf(_T("[SanpoWebPrintWS] RunService %s"), pName);
	// run service with given name
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == 0)
	{
		long nError = GetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenSCManager failed, error code = %d"), nError);
	}
	else
	{
		// open the service
		SC_HANDLE schService = OpenService(schSCManager, pName, SERVICE_ALL_ACCESS);
		if (schService == 0)
		{
			long nError = GetLastError();
			OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenService failed, error code = %d"), nError);
		}
		else
		{
			// call StartService to run the service
			if (StartService(schService, 0, (const TCHAR * *)NULL))
			{
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				return TRUE;
			}
			else
			{
				long nError = GetLastError();
				OutputDebugPrintf(_T("[SanpoWebPrintWS] StartService failed, error code = %d"), nError);
			}
			CloseServiceHandle(schService);
		}
		CloseServiceHandle(schSCManager);
	}
	return FALSE;
}

/*
启动Windows服务
*/
BOOL KillService(TCHAR* pName) {
	// kill service with given name
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == 0)
	{
		long nError = GetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenSCManager failed, error code = %d"), nError);
	}
	else
	{
		// open the service
		SC_HANDLE schService = OpenService(schSCManager, pName, SERVICE_ALL_ACCESS);
		if (schService == 0)
		{
			long nError = GetLastError();
			OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenService failed, error code = %d"), nError);
		}
		else
		{
			// call ControlService to kill the given service
			SERVICE_STATUS status;
			if (ControlService(schService, SERVICE_CONTROL_STOP, &status))
			{
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				return TRUE;
			}
			else
			{
				long nError = GetLastError();
				OutputDebugPrintf(_T("[SanpoWebPrintWS] ControlService failed, error code = %d"), nError);
			}
			CloseServiceHandle(schService);
		}
		CloseServiceHandle(schSCManager);
	}
	return FALSE;
}

/*
安装Windows服务
*/
VOID Install(TCHAR* pPath, TCHAR* pName)
{
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (schSCManager == 0)
	{
		long nError = GetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenSCManager failed, error code = %d"), nError);
	}
	else
	{
		SC_HANDLE schService = CreateService
		(
			//服务控制管理程序维护的登记数据库的句柄，由系统函数OpenSCManager 返回
			schSCManager,
			//以NULL 结尾的服务名，用于创建登记数据库中的关键字
			pName,
			//以NULL 结尾的服务名，用于用户界面标识服务
			pName,
			//指定服务所需的访问
			SERVICE_ALL_ACCESS,
			//指定服务类型
			SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
			//指定何时启动服务
			SERVICE_AUTO_START,      /*系统启动时由服务控制管理器自动启动该服务程序 */
			//指定服务启动失败的严重程度
			SERVICE_ERROR_NORMAL,
			//指定服务程序二进制文件的路径
			pPath,
			//指定顺序装入的服务组名
			NULL,
			//忽略，NULL
			NULL,
			//指定启动该服务前必须先启动的服务或服务组
			NULL,
			//以NULL 结尾的字符串，指定服务帐号。如是NULL,则表示使用LocalSystem帐号
			NULL,
			//以NULL 结尾的字符串，指定对应的口令。为NULL表示无口令。但使用LocalSystem时填NULL
			NULL
		);
		if (schService == 0)
		{
			long nError = GetLastError();
			OutputDebugPrintf(_T("[SanpoWebPrintWS] Failed to create service %s, error code = %d"), pName, nError);
		}
		else
		{
			OutputDebugPrintf(_T("[SanpoWebPrintWS] Service %s installed"), pName);
			CloseServiceHandle(schService);
		}
		CloseServiceHandle(schSCManager);
	}
}

/*
卸载Windows服务
*/
VOID UnInstall(TCHAR* pName) {
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == 0)
	{
		long nError = GetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenSCManager failed, error code = %d"), nError);
	}
	else
	{
		SC_HANDLE schService = OpenService(schSCManager, pName, SERVICE_ALL_ACCESS);
		if (schService == 0)
		{
			long nError = GetLastError();
			OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenService failed, error code = %d"), nError);
		}
		else
		{
			if (!DeleteService(schService))
			{
				OutputDebugPrintf(_T("[SanpoWebPrintWS] Failed to delete service %s"), pName);
			}
			else
			{
				OutputDebugPrintf(_T("[SanpoWebPrintWS] Service %s removed"), pName);
			}
			CloseServiceHandle(schService);
		}
		CloseServiceHandle(schSCManager);
	}
}