#pragma once
#include "framework.h"
#include "pch.h"
#include "Packet.h"

#include <list>
#define BUFFER_SIZE 8192

typedef void (*SOCK_CALLBACK) (void*, int, std::list<CPacket>&, CPacket&);

class CServerSocket
{
public:
    static CServerSocket* getInstance() {
        if (m_instance == NULL) { // ��̬����û��thisָ�룬�޷�ֱ�ӷ��ʳ�Ա����
            m_instance = new CServerSocket();
        }
        return m_instance;
    }

    int Run(SOCK_CALLBACK callback, void* arg, short port = 9527) {
        bool ret = InitServer(port);
        if (ret == false) return -1;
        int count = 0;
        std::list<CPacket> listPacket;
        m_callback = callback;
        m_arg = arg;

        while (true)
        {
            if (AcceptClient() == false) {
                if (count >= 3)
                {
                    return -2;
                }
                count++;
            }
            int ret = DealCommand();
            if (ret > 0)
            {
                m_callback(m_arg, ret, listPacket,m_packet);
                while (listPacket.size() > 0)
                {
                    Send(listPacket.front());
                    listPacket.pop_front();
                }
            }
            CloseClient();
        }
        return 0;
    }
protected:
    // ��ʼ��������
    bool InitServer(short port = 9527) {
        if (m_sock == INVALID_SOCKET) {
            return false;
        }
        sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);
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
        TRACE("[�����] Accept �ȴ����ƶ�����...\n");
        if (m_sock == INVALID_SOCKET) {
            TRACE("�������׽�����Ч��\n");
            return false;
        }

        sockaddr_in cli_addr;
        memset(&cli_addr, 0, sizeof(cli_addr));
        int cli_len = sizeof(cli_addr);
        m_cli_sock = accept(m_sock, (sockaddr*)&cli_addr, &cli_len);

        if (m_cli_sock == INVALID_SOCKET) {
            TRACE("��������ʧ�ܣ������룺%d\n", WSAGetLastError());
            return false;
        }

        TRACE("�յ�������%s����������\n   �˿ںţ�%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        return true;
    }

    // ����ͻ�������
    int DealCommand() {
        if (m_cli_sock == INVALID_SOCKET) {
            return -1;
        }
        TRACE("[�����] DealCommand��ʼ����ͻ��� %d������\n", m_cli_sock);
        char* buffer = new char[BUFFER_SIZE];
        if (buffer == NULL) {
            TRACE("�ڴ����ʧ�ܡ�\n");
            return -2;
        }
        memset(buffer, 0, BUFFER_SIZE);
        static size_t index = 0; // index ��һ����̬���������ڸ��ٻ�������δ�������ݵ���ʼλ��
        while (true) {
            size_t recv_len = recv(m_cli_sock, buffer + index, BUFFER_SIZE - index, 0);

            if (recv_len <= 0) {
                delete[] buffer;
                return -1;
            }
            index += recv_len;
            recv_len = index;
            m_packet = CPacket((BYTE*)buffer, recv_len);//recv_len���룺buffer���ݳ���   recv_len�������ɹ��������ݳ���
            TRACE("[�����] �յ���ͷ��%d, �����ȣ�%d, �������%d, ���ݣ�%s, У��ͣ�%d \n", m_packet.sHead, m_packet.nLength, m_packet.sCmd, m_packet.sData.c_str(), m_packet.sSum);

            if (recv_len > 0) {
                memmove(buffer, buffer + recv_len, BUFFER_SIZE - recv_len);
                //memmove(buffer, buffer + recv_len, index - recv_len);

                index -= recv_len;
                //delete[] buffer;
                return m_packet.sCmd;
            }
        }
        return -1;
    }

    // ��������
    bool Send(char* pData, int nSize) {
        if (m_cli_sock == INVALID_SOCKET) {
            return false;
        }
        int len = send(m_cli_sock, pData, nSize, 0);
        if (len > 0) return true;
        return false;
    }
    bool Send(CPacket& packet) {
        if (m_cli_sock == INVALID_SOCKET) {
            return false;
        }
        //Dump((BYTE*)packet.Data(), packet.Size());
        // ������ݺͳ���
        int dataSize = packet.Size();
        const char* data = packet.Data();
        TRACE("Server package size: %d\r\n", dataSize);

        int len = send(m_cli_sock, data, dataSize, 0);

        if (len == SOCKET_ERROR) {
            int error = WSAGetLastError();
            TRACE("Server Send error = %d\r\n", error);
            return false;
        }
        TRACE("Server Send len = %d\r\n", len);
        return (len > 0);
    }

    // ��ȡ�ļ�·��
    bool GetFilePath(std::string& filePath) {
        if (m_packet.sCmd == 2 || m_packet.sCmd == 3 || m_packet.sCmd == 4 || m_packet.sCmd == 9) {
            filePath = m_packet.sData;
            return true;
        }
        return false;
    }
    // ��ȡ����¼�
    bool GetMouseEvent(MOUSEEV& mouseEvent) {
        if (m_packet.sCmd == 5) {
            memcpy(&mouseEvent, m_packet.sData.c_str(), sizeof(MOUSEEV));
            return true;
        }
    }

    CPacket& GetPacket() {
        return m_packet;
    }

    // �رտͻ�������
    void CloseClient() {
        if (m_cli_sock != INVALID_SOCKET)
        {
            closesocket(m_cli_sock);
            m_cli_sock = INVALID_SOCKET;
        }
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
        TRACE("�������׽��֣�%d\n", m_sock);
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

private:
    SOCK_CALLBACK m_callback;
    void* m_arg;
    static CHelper m_helper;
    static CServerSocket* m_instance;
    SOCKET m_sock;
    SOCKET m_cli_sock;
    CPacket m_packet;
};
