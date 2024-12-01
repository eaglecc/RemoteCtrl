#pragma once
#include "framework.h"
#include "pch.h"

// 解析和处理数据包
class CPacket {
public:
    CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
    // 复制构造函数，用于拷贝数据包
    CPacket(const CPacket& packet) {
        sHead = packet.sHead;
        nLength = packet.nLength;
        sCmd = packet.sCmd;
        sData = packet.sData;
        sSum = packet.sSum;
    }
    // 赋值运算符重载
    CPacket& operator=(const CPacket& packet) {
        if (this == &packet) {
            sHead = packet.sHead;
            nLength = packet.nLength;
            sCmd = packet.sCmd;
            sData = packet.sData;
            sSum = packet.sSum;
        }
        return *this;
    }
    // 构造函数重载，从数据包中解析出各个字段
    CPacket(const BYTE* pData, size_t nSize) {
        size_t i = 0;
        for (; i < nSize; i++) {
            if (*(WORD*)(pData + i) == 0xFEFF) {
                sHead = *(WORD*)(pData + i); // 包头
                i += 2;
                break;
            }
        }
        if (i + 4 + 2 + 2 > nSize) { // 包数据可能不全，或者包头未能全部接收到
            nSize = 0;
            return;
        }
        nLength = *(DWORD*)(pData + i); // 当前包长度
        i += 4;
        if (nLength + i > nSize) { // 包未全部接受到，就返回，解析失败
            nSize = 0;
            return;
        }
        sCmd = *(WORD*)(pData + i); // 控制命令
        i += 2;
        if (nLength > 4) {
            sData.resize(nLength - 2 - 2); // 包数据
            memcpy((void*)sData.c_str(), pData + i, nLength - 4); // 复制数据
            i += nLength - 4;
        }
        sSum = *(WORD*)(pData + i); // 校验和
        i += 2;
        WORD sum = 0; 
        for (size_t j = 0; j < sData.size(); j++) {
            sum += BYTE(sData[i]) & 0xFF;
        }
        if (sum != sSum) { // 校验和错误
            nSize = i;
            return;
        }
        nSize = 0; // 解析失败
    }
    ~CPacket() {}
public:
    WORD sHead; // 包头，固定位： 0xFEFF
    DWORD nLength; // 包长度
    WORD sCmd; // 控制命令
    std::string sData; // 包数据
    WORD sSum; // 和校验
};

class CServerSocket
{
public:
    static CServerSocket* getInstance() {
        if (m_instance == NULL) { // 静态函数没有this指针，无法直接访问成员变量
            m_instance = new CServerSocket();
        }
        return m_instance;
    }

    // 初始化服务器
    bool InitServer() {
        if (m_sock == INVALID_SOCKET) {
            return false;
        }
        sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(9527);
        if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
            return false;
        }
        if (listen(m_sock, 1) == SOCKET_ERROR) {
            return false;
        }
        return true;
    }

    // 等待控制端连接
    bool AcceptClient() {
        sockaddr_in cli_addr;
        memset(&cli_addr, 0, sizeof(cli_addr));
        int cli_len = sizeof(cli_addr);
        m_cli_sock = accept(m_sock, (sockaddr*)&cli_addr, &cli_len);
        if (m_cli_sock == INVALID_SOCKET) {
            return false;
        }
        return true;
    }
#define BUFFER_SIZE 4096
    // 处理控制端命令
    int DealCommand() {
        if (m_cli_sock == INVALID_SOCKET) {
            return -1;
        }
        char* buffer = new char[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        size_t index = 0;
        while (true) {
            size_t recv_len = recv(m_cli_sock, buffer + index, BUFFER_SIZE - index, 0);
            if (recv_len <= 0) {
                return -1;
            }
            index += recv_len;
            recv_len = index;
            m_packet = CPacket((BYTE*)buffer, recv_len);
            if (recv_len > 0) {
                memmove(buffer, buffer + recv_len, BUFFER_SIZE - recv_len);
                index -= recv_len;
                return m_packet.sCmd;
            }
        }
        return -1;
    }

    // 发送数据
    bool Send(const char* pData, int nSize) {
        if (m_cli_sock == INVALID_SOCKET) {
            return false;
        }
        return send(m_cli_sock, (char*)pData, nSize, 0) > 0;
    }

private:
    CServerSocket& operator=(const CServerSocket&) {}
    CServerSocket(const CServerSocket& ss) {
        m_sock = ss.m_sock;
        m_cli_sock = ss.m_cli_sock;
    }

    CServerSocket() {
        m_sock = INVALID_SOCKET;
        m_cli_sock = INVALID_SOCKET;
        if (InitSocketEnv() == FALSE) {
            MessageBox(NULL, _T("无法初始化Socket环境，请检查网络设置"), _T("Error"), MB_OK | MB_ICONERROR);
            exit(0);
        }
        m_sock = socket(PF_INET, SOCK_STREAM, 0);
    }

    ~CServerSocket() {
        closesocket(m_sock);
        WSACleanup();
    }

    // 初始化Socket环境
    BOOL InitSocketEnv() {
        WSADATA data;
        if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
            return FALSE;
        }
        return TRUE;
    }

    static void releaseInstance() {
        if (m_instance != NULL) {
            delete m_instance;
            m_instance = NULL;
        }
    }

    class CHelper {
    public:
        CHelper() {
            CServerSocket::getInstance();
        }
        ~CHelper() {
            CServerSocket::releaseInstance();
        }
    };

    static CHelper m_helper;
    static CServerSocket* m_instance;
    SOCKET m_sock;
    SOCKET m_cli_sock;
    CPacket m_packet;
};

extern CServerSocket server;