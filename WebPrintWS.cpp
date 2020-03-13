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
	//定期检查是否已要求停止服务
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
	//关闭服务器套接字
	if (INVALID_SOCKET != sock) {
		OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceWorkerThread closesocket:%d"), sock);
		closesocket(sock);
	}
	OutputDebugPrintf(_T("[SanpoWebPrintWS] ServiceWorkerThread: Exit"));

	return ERROR_SUCCESS;
}



VOID AcceptSocket() {
	OutputDebugPrintf(_T("[SanpoWebPrintWS] AcceptSocket socket:%d"), sock);
	//设置客户端
	sockaddr_in clientAddr;
	int clientAddrSize = sizeof(clientAddr);
	int clientSock;
	// 接收一个客户机的连接，返回一个socket，来自客户机的socket叫connected socket
	//int accept(int socket,sockaddr * fromaddr,int * addrlen);【阻塞】【仅TCP】【服务器】
	// socket:用来监听的socket（master socket）
	// fromaddr:客户机的地址信息
	// addrlen:地址结构体的长度（输入输出参数）
	// 返回值：返回一个新的socket，这个socket专门用来与此客户机通讯（connected socket）
	if (SOCKET_ERROR != (clientSock = accept(sock, (sockaddr*)& clientAddr, (socklen_t*)& clientAddrSize)))
	{
		OutputDebugPrintf(_T("[SanpoWebPrintWS] AcceptSocket: accept begin"));
		std::string requestStr;
		int bufSize = 4096;
		requestStr.resize(bufSize);
		recv(clientSock, &requestStr[0], bufSize, 0); //接受数据		
		std::string firstLine = requestStr.substr(0, requestStr.find("\r\n"));
		////取得第一行并取得URL以解析确定返回信息
		firstLine = firstLine.substr(firstLine.find(" ") + 1);//substr，复制函数，参数为起始位置（默认0），复制的字符数目
		std::string url = firstLine.substr(0, firstLine.find(" "));
		OutputDebugPrintf(_T("[SanpoWebPrintWS] AcceptSocket: url"));
		std::string response =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html; charset=gbk\r\n"
			"Connection: close\r\n"
			"\r\n";
		send(clientSock, response.c_str(), response.length(), 0);		//发送HTTP响应头
		//std::cout << "\n服务器向客户端浏览器发送响应头为：\n\n" << response;
		OutputDebugPrintf(_T("[SanpoWebPrintWS] 服务器向客户端浏览器发送响应头为"));
		UrlRouter(clientSock, url);	//处理URL			
		closesocket(clientSock);   //关闭客户端套接字
		OutputDebugPrintf(_T("[SanpoWebPrintWS] AcceptSocket accept end"));
	}
}

VOID UrlRouter(int clientSock, std::string const& url)
{
	std::string hint;
	if (url == "/") {
		hint = "文本内容1 ";
	}
	else if (url == "/hello") {
		hint = "文本内容2 ";
	}
	else {
		hint = "未定义URL!";
	}
	send(clientSock, hint.c_str(), hint.length(), 0);// 向的客户端地址回传html文本串 hint
	//std::cout << "服务器已发送：" << hint << std::endl;
	OutputDebugPrintf(_T("[SanpoWebPrintWS] 服务器已发送"));
}

VOID OpenSocket() {
	OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenSocket Entry"));
	WSADATA _wsa;
	//使用socketAPI前，要先将相关链接库（Ws2_32.lib）加入链接，并使用WSAStartUp函数初始化。
	//每个socket函数都可能失败（返回 - 1），需要判断结果
	//套接字初始化，分配套接字版本信息2.0，WSADATA变量地址
	int rc = WSAStartup(MAKEWORD(2, 0), &_wsa);
	if (SOCKET_ERROR == rc) {
		long nError = WSAGetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] WSAStartup failed, error code = %d"), nError);
		return;
	}
	//建立一个socket用于连接
	//int socket(int af, int type, int protocol);
	//af:address family，如AF_INET
	//type : 连接类型，通常是SOCK_STREAM或SOCK_DGRAM
	//protocol : 协议类型，通常是IPPROTO_TCP或IPPROTO_UDP
	// 返回值：socket的编号，为-1表示失败
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == sock) {
		long nError = WSAGetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] create socket failed, error code = %d"), nError);
		return;
	}
	OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenSocket socket:%d"), socket);
	//struct sockaddr_in
	//一个用来指定IP地址和端口号的结构体（不太好用，建议将其封装）
	//family	// 即address family，如AF_INET
	//port	// 端口号（注意要按位倒序，使用htons函数）
	//sin_addr.S_un.S_addr	// 一个为long类型的ip地址
	//该结构体所有成员的字序为网络字序，低字节在前，高字节在后
	sockaddr_in addr = { 0 };
	//指定地址族
	addr.sin_family = AF_INET;
	//IP初始化
	addr.sin_addr.s_addr = INADDR_ANY;
	//端口号初始化为 8081	
	addr.sin_port = htons(42930);
	//分配IP和端口 将一个地址和一个端口号绑定到一个socket连接上
	//int bind(int socket, sockaddr * address, uint addrlen);
	// socket:之前创建的socket
	// sockaddr:一个用来存放Ip地址和端口号的结构体
	// addrlen:上述结构体的长度
	// 返回值：为-1表示失败，若端口被占用，会从新绑定一个随机端口（仍返回失败）
	// 地址绑定为0表示绑定本机所有IP
	int rcBind = bind(sock, (sockaddr*)& addr, sizeof(addr));
	if (SOCKET_ERROR == rcBind) {
		long nError = WSAGetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] bind socket failed, error code = %d"), nError);
		return;
	}
	//设置监听	将一个socket设置为监听状态,专门用来监听的socket叫做master socket
	//int listen(int socket, int maxconn); 【仅TCP】【服务器】
	// 将一个socket设置为监听状态,专门用来监听的socket叫做master socket
	// maxconn:最大接收连接数
	// 返回值：失败返回-1，成功返回0
	int rcListen = listen(sock, 0);
	if (SOCKET_ERROR == rcListen) {
		long nError = WSAGetLastError();
		OutputDebugPrintf(_T("[SanpoWebPrintWS] listen socket failed, error code = %d"), nError);
		return;
	}
	OutputDebugPrintf(_T("[SanpoWebPrintWS] OpenSocket Exit"));
}