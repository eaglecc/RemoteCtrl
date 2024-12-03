﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
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

#include <io.h>
#include <list>
typedef struct file_info {
    file_info() {
        IsInvalid = FALSE;
        memset(szFileName, 0, sizeof(szFileName));
        IsDirectory = -1;
        HasNext = FALSE;
    }
    BOOL IsInvalid; // 是否是无效文件
    char szFileName[256]; // 文件名
    BOOL IsDirectory; // 是否是目录
    BOOL HasNext; // 是否有下一个文件
}FILEINFO, * PFILEINFO;

// 创建指定目录下的文件信息
int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFO> fileInfoLists;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("无法获取文件路径"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) { // 将当前进程的工作目录更改为 strPath 指定的路径
        FILEINFO fileInfo;
        fileInfo.IsInvalid = TRUE;
        fileInfo.IsDirectory = TRUE;
        fileInfo.HasNext = FALSE;
        memcpy(fileInfo.szFileName, strPath.c_str(), strPath.size());
        //fileInfoLists.push_back(fileInfo);
        CPacket pack((WORD)2, (BYTE*)&fileInfo, (size_t)sizeof(fileInfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限访问目录"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("无法找到文件"));
        return -3;
    }
    do
    {
        FILEINFO fileInfo;
        fileInfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(fileInfo.szFileName, fdata.name, strlen(fdata.name));
        //fileInfoLists.push_back(fileInfo);
        CPacket pack((WORD)2, (BYTE*)&fileInfo, (size_t)sizeof(fileInfo));
        CServerSocket::getInstance()->Send(pack);
    } while (!_findnext(hfind, &fdata));
    // 发送信息到客户端
    FILEINFO fileInfo;
    fileInfo.HasNext = FALSE;
    return 0;
}

// 打开文件
int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    CPacket pack((WORD)3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

// 下载文件
int downloadFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (err != 0) {
        CPacket pack((WORD)4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile); // 整个文件的大小
        CPacket head(4, (BYTE*)&data, 8);
        fseek(pFile, 0, SEEK_SET);

        char buffer[1024] = { 0 };
        size_t rLen = 0;
        do {
            rLen = fread(buffer, 1, sizeof(buffer), pFile);
            CPacket pack((WORD)4, (BYTE*)buffer, rLen);
            CServerSocket::getInstance()->Send(pack);
        } while (rLen >= sizeof(buffer));
        fclose(pFile);
    }
    CPacket pack((WORD)4, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

// 鼠标控制
int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
        DWORD nFlags = 0;
        switch (mouse.nButton)
        {
        case 0:// 左键
            nFlags = 1;
            break;
        case 1:// 右键
            nFlags = 2;
            break;
        case 2:// 中键
            nFlags = 4;
            break;
        case 3:// 没有按键
            nFlags = 8;
            break;
        default:
            break;
        }

        if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);

        switch (mouse.nAction) {
        case 0: // 单击
            nFlags |= 0x10;
            break;
        case 1: // 双击
            nFlags |= 0x20;
            break;
        case 2: // 按下
            nFlags |= 0x40;
            break;
        case 3: // 弹起
            nFlags |= 0x80;
            break;
        default:
            break;
        }
        switch (nFlags)
        {
        case 0x21: // 左键双击(利用switch 穿透效果)
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11: // 左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41: // 左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81: // 左键弹起
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22: // 右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12: // 右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42: // 右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82: // 右键弹起
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24: // 中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14: // 中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x44: // 中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84: // 中键弹起
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08: // 单纯的鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, 0, 0, 0, GetMessageExtraInfo());
            break;
        default:
            break;
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    else {
        OutputDebugString(_T("获取鼠标事件失败"));
        return -1;
    }
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
            case 1:// 查看磁盘信息
                MakeDriverInfo();
                break;
            case 2: // 查看指定目录下的文件
                MakeDirectoryInfo();
                break;
            case 3: // 打开文件
                RunFile();
                break;
            case 4: // 下载文件
                downloadFile();
                break;
            case 5: // 鼠标控制
                MouseEvent();
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
