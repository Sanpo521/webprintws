#include "stdafx.h"
#include "ServiceCtrl.h"
#include "DebugInfoPrint.h"

/*
����Windows����
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
����Windows����
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
��װWindows����
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
			//������ƹ������ά���ĵǼ����ݿ�ľ������ϵͳ����OpenSCManager ����
			schSCManager,
			//��NULL ��β�ķ����������ڴ����Ǽ����ݿ��еĹؼ���
			pName,
			//��NULL ��β�ķ������������û������ʶ����
			pName,
			//ָ����������ķ���
			SERVICE_ALL_ACCESS,
			//ָ����������
			SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
			//ָ����ʱ��������
			SERVICE_AUTO_START,      /*ϵͳ����ʱ�ɷ�����ƹ������Զ������÷������ */
			//ָ����������ʧ�ܵ����س̶�
			SERVICE_ERROR_NORMAL,
			//ָ���������������ļ���·��
			pPath,
			//ָ��˳��װ��ķ�������
			NULL,
			//���ԣ�NULL
			NULL,
			//ָ�������÷���ǰ�����������ķ���������
			NULL,
			//��NULL ��β���ַ�����ָ�������ʺš�����NULL,���ʾʹ��LocalSystem�ʺ�
			NULL,
			//��NULL ��β���ַ�����ָ����Ӧ�Ŀ��ΪNULL��ʾ�޿����ʹ��LocalSystemʱ��NULL
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
ж��Windows����
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