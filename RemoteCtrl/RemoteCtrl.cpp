// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"

#include <direct.h>
#include <atlimage.h>

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
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

// 创建磁盘分区信息
int MakeDriverInfo() { // 1==> A盘, 2==>B��, 3==>C盘...,26=>Z盘
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

// 创建指定目录下的文件信息
int MakeDirectoryInfo() {
    std::string strPath;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("无法获取文件路径"));
        return -1;
    }
    //设置为当前工作目录
    if (!SetCurrentDirectoryA(strPath.c_str()))
    {
        //设置失败
        FILEINFO finfo;
        finfo.HasNext = FALSE;//没有后续文件
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));//打包
        CServerSocket::getInstance()->Send(pack);//发送
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
        CPacket pack((WORD)2, (BYTE*)&fileInfo, (size_t)sizeof(fileInfo));
        CServerSocket::getInstance()->Send(pack);
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
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));//打包
        CServerSocket::getInstance()->Send(pack);//发送
        Count++;
        TRACE("[服务端] [MakeDirectoryInfo] [拿到的文件信息名：] %s\r\n", finfo.szFileName);
        TRACE("[服务器数据包：]\r\n");
        Dump((BYTE*)pack.Data(), pack.Size());
    } while (_findnext(hfind, &fdata) == 0);
    //while (!_findnext(hfind, &fdata)); //查找工作目录匹配的下一个文件

    // 发送信息到客户端
    FILEINFO fileInfo;
    fileInfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&fileInfo, sizeof(fileInfo));
    CServerSocket::getInstance()->Send(pack);

    TRACE("Count=%d\r\n", Count);
    _findclose(hfind); // 关闭查找句柄
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
        // 首先将文件指针移动到文件末尾，获取文件大小并发送给客户端。
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile); // 整个文件的大小
        CPacket head(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(head);
        fseek(pFile, 0, SEEK_SET);// 将文件指针移动到文件开头

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
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    else {
        OutputDebugString(_T("获取鼠标事件失败"));
        return -1;
    }
}

// 发送屏幕内容
int SendScreen() {
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
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);//内存块解锁
    }
    pStream->Release();//释放流
    GlobalFree(hMem);//释放内存块
    screen.ReleaseDC();
    return 0;
}

#include "LockDialog.h"
CLockDialog dlg;
unsigned int threadId = 0;

unsigned __stdcall threadLockDlg(void* arg) {
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
    dlg.DestroyWindow();
    _endthreadex(0); // 结束线程
    return 0;
}

// 锁机
int LockMachine() {
    if (dlg.m_hWnd != NULL || dlg.m_hWnd != INVALID_HANDLE_VALUE) {
        //_beginthread(threadLockDlg, 0, NULL);
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadId);
    }
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

// 解锁
int UnmLockMachine() {
    //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, VK_ESCAPE, 0); // 发送ESC键退出锁屏
    PostThreadMessage(threadId, WM_KEYDOWN, VK_ESCAPE, 0); // 发送ESC键退出锁屏
    CPacket pack(8, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

// 删除文件
int DeleteLocalFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    TCHAR sPath[MAX_PATH] = _T("");
    //mbstowcs(sPath, strPath.c_str(), strPath.size()); // 多字节转宽字节 中文容易乱码
    MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(),
        sPath, sizeof(sPath) / sizeof(TCHAR));
    DeleteFileA(strPath.c_str());
    CPacket pack(9, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);

    return 0;
}

// 连接测试
int TestConnect() {
    CPacket pack(2000, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

// 接受命令并处理
int ExcuteCommand(int nCmd) {
    int ret = 0;
    switch (nCmd)
    {
    case 1:// 查看磁盘信息
        ret = MakeDriverInfo();
        break;
    case 2: // 查看指定目录下的文件
        ret = MakeDirectoryInfo();
        break;
    case 3: // 打开文件
        ret = RunFile();
        break;
    case 4: // 下载文件
        ret = downloadFile();
        break;
    case 5: // 鼠标控制
        ret = MouseEvent();
        break;
    case 6: // 发送屏幕内容==>截屏
        ret = SendScreen();
        break;
    case 7: // 锁机
        ret = LockMachine();
        break;
    case 8: // 解锁
        ret = UnmLockMachine();
        break;
    case 9: // 删除文件
        ret = DeleteLocalFile();
        break;
    case 2000: // 连接测试
        ret = TestConnect();
        break;
    default:
        break;
    }
    return ret;
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
                    ret = ExcuteCommand(ret);
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
