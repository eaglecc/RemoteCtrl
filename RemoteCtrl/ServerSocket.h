#pragma once
#include "framework.h"
#include "pch.h"

class CServerSocket
{
public:
    static CServerSocket* getInstance() {
        if (m_instance == NULL) { // 静态函数没有this指针，无法直接访问成员变量
            m_instance = new CServerSocket();
        }
        return m_instance;
    };
private:
    CServerSocket& operator=(const CServerSocket&) {}
    CServerSocket(const CServerSocket&) {}

    CServerSocket() {
        if (InitSocketEnv() == FALSE) {
            MessageBox(NULL, _T("无法初始化Socket环境，请检查网络设置"), _T("Error"), MB_OK | MB_ICONERROR);
            exit(0);
        }
    }

    ~CServerSocket() {
        WSACleanup();
    }

    BOOL InitSocketEnv() {
        WSADATA data;
        if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
            return FALSE;
        }
        return TRUE;
    }

    static void releaseInstance() {
        if (m_instance!= NULL) {
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
};

extern CServerSocket server;