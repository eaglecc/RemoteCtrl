// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "CETool.h"
#include "CCommand.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//#pragma comment(linker,"/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment(linker,"/subsystem:windows /entry:mainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:WinMainCRTStartup")

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
            CCommand command;
            // 1. 初始化网络
            CServerSocket* pserver = CServerSocket::getInstance();
            if (pserver->InitServer() == false) {
                MessageBox(NULL, L"网络初始化失败", L"错误", MB_OK | MB_ICONERROR);
                exit(0);
            }
            int count = 0;
            while (pserver != nullptr) {
                // 2. 等待客户端连接
                if (pserver->AcceptClient() == false) {
                    TRACE("接受客户端连接失败，错误码：%d\n", WSAGetLastError());
                    if (count > 3) {
                        MessageBox(NULL, L"自动重试超过3次，程序结束！", L"错误", MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, L"无法接受客户端连接，自动重试", L"错误", MB_OK | MB_ICONERROR);
                    count++;
                    Sleep(1000);
                    continue;
                }
                TRACE("[服务端] [main] 客户端连接成功\r\n");
                // 3. 处理客户端命令
                int ret = pserver->DealCommand();
                TRACE("[服务端] 服务器处理命令：%d\r\n", ret);
                if (ret > 0)
                {
                    ret = command.ExcuteCommand(ret);
                    if (ret != 0) {
                        TRACE("[服务端] 命令执行失败：%d\r\n", ret);
                    }
                    pserver->CloseClient();
                }
            }
            // TODO: 4. 释放网络资源
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
