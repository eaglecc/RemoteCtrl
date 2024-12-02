// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>

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
// 将给定的字节数据以十六进制的形式输出到调试窗口
void Dump(BYTE* pData, size_t nSize) {
    std::string strOut;
    for (size_t i = 0; i < nSize; i++) {
        char buf[8] = { 0 };
        if (i > 0 && i % 16 == 0) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

// 创建磁盘分区信息
int MakeDriverInfo() { // 1==> A盘, 2==>B盘, 3==>C盘...,26=>Z盘
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            if (result.size() > 0) {
                result += ',';
            }
            result += 'A' + i - 1;
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size()); // 磁盘信息数据打包
    Dump((BYTE*)pack.Data(), pack.Size());
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

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
            //// 1. 初始化网络
            //CServerSocket* pserver = CServerSocket::getInstance();
            //if (pserver->InitServer() == false) {
            //    MessageBox(NULL, L"网络初始化失败", L"错误", MB_OK | MB_ICONERROR);
            //    exit(0);
            //}
            //int count = 0;
            //while (pserver != nullptr) {
            //    // 2. 等待客户端连接
            //    if (pserver->AcceptClient() == false) {
            //        if (count > 3) {
            //            MessageBox(NULL, L"自动重试超过3次，程序结束！", L"错误", MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, L"无法接受客户端连接，自动重试", L"错误", MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    // 3. 处理客户端命令
            //    int ret = pserver->DealCommand();
            //
            //}

            int nCmd = 1;
            switch (nCmd)
            {
            case 1:
                // 查看磁盘信息
                MakeDriverInfo();
                break;
            default:
                break;
            }


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
