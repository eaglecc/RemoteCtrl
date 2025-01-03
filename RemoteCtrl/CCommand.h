#pragma once
#include "ServerSocket.h"
#include "LockDialog.h"
#include "CETool.h"
#include "LockDialog.h"
#include "Resource.h"
#include "Packet.h"

#include <direct.h>
#include <atlimage.h>
#include <map>
#include <io.h>
#include <list> 

class CCommand
{
public:
    CCommand();
    ~CCommand();
    int ExcuteCommand(int nCmd, std::list<CPacket>& listPacket, CPacket& inPacket);
    static void RunCommand(void* arg, int status, std::list<CPacket>& listPacket, CPacket& inPacket) {
        CCommand* thiz = (CCommand*)arg;
        if (status > 0) {
            int ret = thiz->ExcuteCommand(status, listPacket, inPacket);
            if (ret != 0) {
                TRACE("[服务端] [CCommand.h]命令执行失败：%d\r\n", ret);
            }
        }
        else
        {
            MessageBox(NULL, L"无法接入用户", L"接入用户失败！", MB_OK | MB_ICONERROR);
        }
    }

protected:
    typedef int (CCommand::* CMDFUNC) (std::list<CPacket>&, CPacket& inPacket); // 成员函数指针
    std::map<int, CMDFUNC> m_mapFunction; // 从命令号到函数的映射表
    CLockDialog dlg; // 锁机窗口
    unsigned int threadId; // 线程ID

protected:
    static unsigned __stdcall threadLockDlg(void* arg) {
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
        _endthreadex(0); // 结束线程
        return 0;
    }

    void threadLockDlgMain() {
        dlg.Create(IDD_DIALOG_INFO, NULL);
        dlg.ShowWindow(SW_SHOW);
        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); //窗口置顶
        ShowCursor(FALSE); // 隐藏鼠标
        //::ShowWindow(::FindWindow(L"Shell_TrayWnd", NULL), SW_HIDE); // 隐藏任务栏
        CRect rect;
        dlg.GetWindowRect(&rect); // 获取窗口位置
        ClipCursor(rect); // 限制鼠标位置 

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) { // 消息循环
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            // 处理键盘事件
            if (msg.message == WM_KEYDOWN) {
                if (msg.wParam == VK_ESCAPE) {
                    break;
                }
            }
        }
        ClipCursor(NULL); // 释放鼠标
        ShowCursor(TRUE); // 恢复鼠标
        //::ShowWindow(::FindWindow(L"Shell_TrayWnd", NULL), SW_SHOW); // 恢复任务栏
        dlg.DestroyWindow(); // 销毁窗口
    }

    // 创建磁盘分区信息
    int MakeDriverInfo(std::list<CPacket>& listPacket, CPacket& inPacket) { // 1==> A盘, 2==>B盘, 3==>C盘...,26=>Z盘
        std::string result;
        for (int i = 1; i <= 26; i++) {
            if (_chdrive(i) == 0) {
                if (result.size() > 0) {
                    result += ',';
                }
                result += 'A' + i - 1;
            }
        }
        listPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));  // 磁盘信息数据打包
        return 0;
    }


    // 创建指定目录下的文件信息
    int MakeDirectoryInfo(std::list<CPacket>& listPacket, CPacket& inPacket) {
        std::string strPath = inPacket.sData;
        //设置为当前工作目录
        if (!SetCurrentDirectoryA(strPath.c_str()))
        {
            //设置失败
            FILEINFO finfo;
            finfo.HasNext = FALSE;//没有后续文件
            listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            OutputDebugString(TEXT("没有权限访问目录"));
            return -2;
        }
        //设置当前工作目录成功
        struct _finddata_t fdata;

        std::string searchPath = strPath + "*";
        long long hfind = _findfirst(searchPath.c_str(), &fdata);

        //long long hfind = _findfirst("*", &fdata); //找工作目录中匹配的第一个文件  第一个参数使用通配符代表文件类型
        if (hfind == -1) {
            OutputDebugString(_T("没有找到任何文件"));
            FILEINFO fileInfo;
            fileInfo.HasNext = FALSE;
            listPacket.push_back(CPacket(2, (BYTE*)&fileInfo, (size_t)sizeof(fileInfo)));
            return -3;
        }

        int Count = 0;
        //挨个发送有效文件给客户端
        do {
            FILEINFO finfo;
            finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
            //strncpy(finfo.szFileName, fdata.name, sizeof(finfo.szFileName) - 1);
            //finfo.szFileName[sizeof(finfo.szFileName) - 1] = '\0';
            TRACE("[服务端] finfo.szFileName: %s , fdata.name: %s \r\n", finfo.szFileName, fdata.name);
            listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            Count++;
            TRACE("[服务端] [MakeDirectoryInfo] [拿到的文件信息名：] %s\r\n", finfo.szFileName);
            TRACE("[服务器数据包：]\r\n");
            //CETool::Dump((BYTE*)pack.Data(), pack.Size());
        } while (_findnext(hfind, &fdata) == 0);
        //while (!_findnext(hfind, &fdata)); //查找工作目录匹配的下一个文件

        // 发送信息到客户端
        FILEINFO fileInfo;
        fileInfo.HasNext = FALSE;
        listPacket.push_back(CPacket(2, (BYTE*)&fileInfo, sizeof(fileInfo)));
        TRACE("Count=%d\r\n", Count);
        _findclose(hfind); // 关闭查找句柄
        return 0;
    }

    // 打开文件
    int RunFile(std::list<CPacket>& listPacket, CPacket& inPacket) {
        std::string strPath = inPacket.sData;
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        listPacket.push_back(CPacket((WORD)3, NULL, 0));
        return 0;
    }

    // 下载文件
    int downloadFile(std::list<CPacket>& listPacket, CPacket& inPacket) {
        std::string strPath = inPacket.sData;
        long long data = 0;
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
        if (err != 0) {
            listPacket.push_back(CPacket((WORD)4, (BYTE*)&data, 8));
            return -1;
        }
        if (pFile != NULL) {
            // 首先将文件指针移动到文件末尾，获取文件大小并发送给客户端。
            fseek(pFile, 0, SEEK_END);
            data = _ftelli64(pFile); // 整个文件的大小
            listPacket.push_back(CPacket((WORD)4, (BYTE*)&data, 8));
            fseek(pFile, 0, SEEK_SET);// 将文件指针移动到文件开头

            char buffer[1024] = { 0 };
            size_t rLen = 0;
            do {
                rLen = fread(buffer, 1, sizeof(buffer), pFile);
                listPacket.push_back(CPacket((WORD)4, (BYTE*)buffer, rLen));
            } while (rLen >= sizeof(buffer));
            fclose(pFile);
        }
        listPacket.push_back(CPacket((WORD)4, NULL, 0));
        return 0;
    }

    // 鼠标控制
    int MouseEvent(std::list<CPacket>& listPacket, CPacket& inPacket) {
        MOUSEEV mouse;
        memcpy(&mouse, inPacket.sData.c_str(), sizeof(MOUSEEV));

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
        TRACE("[服务端] [MouseEvent] [nFlags: %d] [nAction: %d] [nButton: %d] [ptXY.x: %d] [ptXY.y: %d]\r\n", nFlags, mouse.nAction, mouse.nButton, mouse.ptXY.x, mouse.ptXY.y);
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
        listPacket.push_back(CPacket(5, NULL, 0));
        return 0;
    }

    // 发送屏幕内容
    int SendScreen(std::list<CPacket>& listPacket, CPacket& inPacket) {
        //屏幕截图
        HDC hScreen = ::GetDC(NULL);//获取屏幕上下文句柄（屏幕截图）
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//RGB位深度 R8位 G8位 B8位 共24位
        int nWidth = GetDeviceCaps(hScreen, HORZRES);
        int nHeight = GetDeviceCaps(hScreen, VERTRES);

        //创建一个图像对象
        CImage screen;
        screen.Create(nWidth, nHeight, nBitPerPixel);

        //BitBlt(screen.GetDC(), 0, 0, 1920, 1020, hScreen, 0, 0, SRCCOPY);//将hScreen图像复制到screen图像中
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);//将hScreen图像复制到screen图像中

        //删除屏幕截图
        ReleaseDC(NULL, hScreen);

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//分配一个可移动的内存块
        if (hMem == NULL)return -1;
        IStream* pStream = NULL;//声明一个指向IStream接口的指针
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//基于内存块创建一个内存流，pStream指向流对象
        if (ret == S_OK)
        {
            //screen.Save(pStream, Gdiplus::ImageFormatPNG);// 将图片保存到内存流中 PNG
            screen.Save(pStream, Gdiplus::ImageFormatJPEG); // 保存图像到内存流 JPEG
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);//将流指针移到流的起始位置
            PBYTE pData = (PBYTE)GlobalLock(hMem);//锁定内存块，转化为字节型指针，获取内存块的起始地址
            SIZE_T nSize = GlobalSize(hMem);//获取分配内存块大小
            listPacket.push_back(CPacket(6, pData, nSize));
            GlobalUnlock(hMem);//内存块解锁
        }
        pStream->Release();//释放流
        GlobalFree(hMem);//释放内存块
        screen.ReleaseDC();
        return 0;
    }


    // 锁机
    int LockMachine(std::list<CPacket>& listPacket, CPacket& inPacket) {
        if (dlg.m_hWnd != NULL || dlg.m_hWnd != INVALID_HANDLE_VALUE) {
            //_beginthread(threadLockDlg, 0, NULL);
            _beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadId);
        }
        listPacket.push_back(CPacket(7, NULL, 0));
        return 0;
    }

    // 解锁
    int UnmLockMachine(std::list<CPacket>& listPacket, CPacket& inPacket) {
        //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, VK_ESCAPE, 0); // 发送ESC键退出锁屏
        PostThreadMessage(threadId, WM_KEYDOWN, VK_ESCAPE, 0); // 发送ESC键退出锁屏
        listPacket.push_back(CPacket(8, NULL, 0));
        return 0;
    }

    // 删除文件
    int DeleteLocalFile(std::list<CPacket>& listPacket, CPacket& inPacket) {
        std::string strPath = inPacket.sData;
        TCHAR sPath[MAX_PATH] = _T("");
        //mbstowcs(sPath, strPath.c_str(), strPath.size()); // 多字节转宽字节 中文容易乱码
        MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(),
            sPath, sizeof(sPath) / sizeof(TCHAR));
        DeleteFileA(strPath.c_str());
        listPacket.push_back(CPacket(9, NULL, 0));
        return 0;
    }

    // 连接测试
    int TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket) {
        listPacket.push_back(CPacket(2000, NULL, 0));
        return 0;
    }
};

