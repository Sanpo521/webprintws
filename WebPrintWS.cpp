#include "stdafx.h"
#include "DebugInfoPrint.h"
#include "WebPrintWS.h"
#include "GlobalVariables.h"

HANDLE				   g_ServiceStopEvent = INVALID_HANDLE_VALUE;
SERVICE_STATUS_HANDLE   g_StatusHandle = NULL;
SERVICE_STATUS          g_ServiceStatus = { 0 };

#define LISTEN_MAXCONN  255
int sock = INVALID_SOCKET;
VOID OpenSocket();
VOID AcceptSocket();
VOID UrlRouter(int clientSock, std::string const& url);

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceWorkerThread: Entry"));
	OpenSocket();
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceWorkerThread socket:%d"), sock);
	//���ڼ���Ƿ���Ҫ��ֹͣ����
	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(sock, &fd);
	timeval timeout = { 0, 100 };
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		fd_set fdOld = fd;
		int iResult = select(0, &fdOld, NULL, NULL, &timeout);
		if (0 <= iResult) {
			AcceptSocket();
		}
	}
	//�رշ������׽���
	if (INVALID_SOCKET != sock) {
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceWorkerThread closesocket:%d"), sock);
		closesocket(sock);
	}
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceWorkerThread: Exit"));

	return ERROR_SUCCESS;
}



VOID AcceptSocket() {
	OutputDebugPrintf(_T("[SanpoWebPrintWS] AcceptSocket socket:%d"), sock);
	//���ÿͻ���
	sockaddr_in clientAddr;
	int clientAddrSize = sizeof(clientAddr);
	int clientSock;
	// ����һ���ͻ��������ӣ�����һ��socket�����Կͻ�����socket��connected socket
	//int accept(int socket,sockaddr * fromaddr,int * addrlen);������������TCP������������
	// socket:����������socket��master socket��
	// fromaddr:�ͻ����ĵ�ַ��Ϣ
	// addrlen:��ַ�ṹ��ĳ��ȣ��������������
	// ����ֵ������һ���µ�socket�����socketר��������˿ͻ���ͨѶ��connected socket��
	if (SOCKET_ERROR != (clientSock = accept(sock, (sockaddr*)& clientAddr, (socklen_t*)& clientAddrSize)))
	{
		OutputDebugPrintf(_T("[SanpoWebPrintWS] AcceptSocket: accept begin"));
		std::string requestStr;
		int bufSize = 4096;
		requestStr.resize(bufSize);
		recv(clientSock, &requestStr[0], bufSize, 0); //��������		
		std::string firstLine = requestStr.substr(0, requestStr.find("\r\n"));
		////ȡ�õ�һ�в�ȡ��URL�Խ���ȷ��������Ϣ
		firstLine = firstLine.substr(firstLine.find(" ") + 1);//substr�����ƺ���������Ϊ��ʼλ�ã�Ĭ��0�������Ƶ��ַ���Ŀ
		std::string url = firstLine.substr(0, firstLine.find(" "));
		OutputDebugPrintf(_T("[SanpoWebPrintWS] AcceptSocket: url"));
		std::string response =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html; charset=gbk\r\n"
			"Connection: close\r\n"
			"\r\n";
		send(clientSock, response.c_str(), response.length(), 0);		//����HTTP��Ӧͷ
		//std::cout << "\n��������ͻ��������������ӦͷΪ��\n\n" << response;
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ��������ͻ��������������ӦͷΪ"));
		UrlRouter(clientSock, url);	//����URL			
		closesocket(clientSock);   //�رտͻ����׽���
		OutputDebugPrintf(_T("[SanpoWebPrintWS] AcceptSocket accept end"));
	}
}

VOID UrlRouter(int clientSock, std::string const& url)
{
	std::string hint;
	if (url == "/") {
		hint = "�ı�����1 ";
	}
	else if (url == "/hello") {
		hint = "�ı�����2 ";
	}
	else {
		hint = "δ����URL!";
	}
	send(clientSock, hint.c_str(), hint.length(), 0);// ��Ŀͻ��˵�ַ�ش�html�ı��� hint
	//std::cout << "�������ѷ��ͣ�" << hint << std::endl;
	OutputDebugPrintf(_T("[SanpoWebPrintWS] �������ѷ���"));
}

VOID OpenSocket() {
	OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenSocket Entry"));
	WSADATA _wsa;
	//ʹ��socketAPIǰ��Ҫ�Ƚ�������ӿ⣨Ws2_32.lib���������ӣ���ʹ��WSAStartUp������ʼ����
	//ÿ��socket����������ʧ�ܣ����� - 1������Ҫ�жϽ��
	//�׽��ֳ�ʼ���������׽��ְ汾��Ϣ2.0��WSADATA������ַ
	int rc = WSAStartup(MAKEWORD(2, 0), &_wsa);
	if (SOCKET_ERROR == rc) {
		long nError = WSAGetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] WSAStartup failed, error code = %d"), nError);
		return;
	}
	//����һ��socket��������
	//int socket(int af, int type, int protocol);
	//af:address family����AF_INET
	//type : �������ͣ�ͨ����SOCK_STREAM��SOCK_DGRAM
	//protocol : Э�����ͣ�ͨ����IPPROTO_TCP��IPPROTO_UDP
	// ����ֵ��socket�ı�ţ�Ϊ-1��ʾʧ��
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == sock) {
		long nError = WSAGetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] create socket failed, error code = %d"), nError);
		return;
	}
	OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenSocket socket:%d"), socket);
	//struct sockaddr_in
	//һ������ָ��IP��ַ�Ͷ˿ںŵĽṹ�壨��̫���ã����齫���װ��
	//family	// ��address family����AF_INET
	//port	// �˿ںţ�ע��Ҫ��λ����ʹ��htons������
	//sin_addr.S_un.S_addr	// һ��Ϊlong���͵�ip��ַ
	//�ýṹ�����г�Ա������Ϊ�������򣬵��ֽ���ǰ�����ֽ��ں�
	sockaddr_in addr = { 0 };
	//ָ����ַ��
	addr.sin_family = AF_INET;
	//IP��ʼ��
	addr.sin_addr.s_addr = INADDR_ANY;
	//�˿ںų�ʼ��Ϊ 8081	
	addr.sin_port = htons(42930);
	//����IP�Ͷ˿� ��һ����ַ��һ���˿ںŰ󶨵�һ��socket������
	//int bind(int socket, sockaddr * address, uint addrlen);
	// socket:֮ǰ������socket
	// sockaddr:һ���������Ip��ַ�Ͷ˿ںŵĽṹ��
	// addrlen:�����ṹ��ĳ���
	// ����ֵ��Ϊ-1��ʾʧ�ܣ����˿ڱ�ռ�ã�����°�һ������˿ڣ��Է���ʧ�ܣ�
	// ��ַ��Ϊ0��ʾ�󶨱�������IP
	int rcBind = bind(sock, (sockaddr*)& addr, sizeof(addr));
	if (SOCKET_ERROR == rcBind) {
		long nError = WSAGetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] bind socket failed, error code = %d"), nError);
		return;
	}
	//���ü���	��һ��socket����Ϊ����״̬,ר������������socket����master socket
	//int listen(int socket, int maxconn); ����TCP������������
	// ��һ��socket����Ϊ����״̬,ר������������socket����master socket
	// maxconn:������������
	// ����ֵ��ʧ�ܷ���-1���ɹ�����0
	int rcListen = listen(sock, 0);
	if (SOCKET_ERROR == rcListen) {
		long nError = WSAGetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] listen socket failed, error code = %d"), nError);
		return;
	}
	OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenSocket Exit"));
}