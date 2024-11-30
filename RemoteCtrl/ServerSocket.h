#pragma once
#include "framework.h"
#include "pch.h"

class CServerSocket
{
public:
    static CServerSocket* getInstance() {
        if (m_instance == NULL) { // ��̬����û��thisָ�룬�޷�ֱ�ӷ��ʳ�Ա����
            m_instance = new CServerSocket();
        }
        return m_instance;
    }

    // ��ʼ��������
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

    // �ȴ����ƶ�����
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

    // ������ƶ�����
    int DealCommand() {
        if (m_cli_sock == INVALID_SOCKET) {
            return -1;
        }
        char recv_buf[1024] = { 0 };
        while (true) {
            int recv_len = recv(m_cli_sock, recv_buf, sizeof(recv_buf), 0);
            if (recv_len <= 0) {
                return -1;
            }
            // TODO: ��������
        }
    }

    // ��������
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
            MessageBox(NULL, _T("�޷���ʼ��Socket������������������"), _T("Error"), MB_OK | MB_ICONERROR);
            exit(0);
        }
        m_sock = socket(PF_INET, SOCK_STREAM, 0);
    }

    ~CServerSocket() {
        closesocket(m_sock);
        WSACleanup();
    }

    // ��ʼ��Socket����
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
};

extern CServerSocket server;