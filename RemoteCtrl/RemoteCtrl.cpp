// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: socket、bind、listen、accept、read、write、close
            // 1. 套接字初始化
            
            //server; // server 是在main函数之间进行初始化的
            // 2. 绑定本地地址
            SOCKET serv_sock = socket(PF_INET, SOCK_STREAM, 0);
            sockaddr_in serv_addr, cli_addr;
            memset(&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            serv_addr.sin_port = htons(9527);
            // 3. 监听
            bind(serv_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
            listen(serv_sock, 1);
            // 4. 等待客户端连接
            char recv_buf[1024] = { 0 };
            int cli_len = sizeof(cli_addr);
            //SOCKET cli_sock = accept(serv_sock, (sockaddr*)&cli_addr, &cli_len);
            // 5. 接收数据
            //recv(cli_sock, recv_buf, sizeof(recv_buf), 0);
            //send(cli_sock, recv_buf, sizeof(recv_buf), 0);
            closesocket(serv_sock);
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
