#pragma once
#include <string>

#pragma pack(push)
#pragma pack(1)
// �����ʹ������ݰ�
class CPacket {
public:
    CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
    CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
        sHead = 0xFEFF;
        nLength = nSize + 2 + 2; // ����(2) + ����(nSize) + У���(2)
        sCmd = nCmd;
        if (nSize > 0) {
            sData.resize(nSize);
            memcpy((void*)sData.c_str(), pData, nSize);
        }
        else {
            sData.clear();
        }
        sSum = 0;
        for (size_t i = 0; i < sData.size(); i++) {
            BYTE  vlaue = BYTE(sData[i]) & 0xFF;
            sSum += vlaue;
        }
        sSum = (WORD)(sSum & 0xFFFF);
    }
    // ���ƹ��캯�������ڿ������ݰ�
    CPacket(const CPacket& packet) {
        sHead = packet.sHead;
        nLength = packet.nLength;
        sCmd = packet.sCmd;
        sData = packet.sData;
        sSum = packet.sSum;
    }
    // ��ֵ���������
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
    // ���캯�����أ������ݰ��н����������ֶ�
    CPacket(const BYTE* pData, size_t nSize) {
        size_t i = 0;
        for (; i < nSize; i++) {
            if (*(WORD*)(pData + i) == 0xFEFF) {
                sHead = *(WORD*)(pData + i); // ��ͷ
                i += 2;
                break;
            }
        }
        if (i + 4 + 2 + 2 > nSize) { // �����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ�����յ�
            nSize = 0;
            return;
        }
        nLength = *(DWORD*)(pData + i); // ��ǰ������
        i += 4;
        if (nLength + i > nSize) { // ��δȫ�����ܵ����ͷ��أ�����ʧ��
            nSize = 0;
            return;
        }
        sCmd = *(WORD*)(pData + i); // ��������
        i += 2;
        if (nLength > 4) {
            sData.resize(nLength - 2 - 2); // ������
            memcpy((void*)sData.c_str(), pData + i, nLength - 4); // ��������
            i += nLength - 4;
        }
        sSum = *(WORD*)(pData + i); // У���
        i += 2;
        WORD sum = 0;
        for (size_t j = 0; j < sData.size(); j++) {
            sum += BYTE(sData[j]) & 0xFF;
        }
        if (sum != sSum) { // У��ʹ���
            nSize = i;
            return;
        }
        nSize = 0; // ����ʧ��
    }

    // �����ݴ�С
    int Size() {
        return nLength + 2 + 4;
    }
    const char* Data() {
        strOut.resize(nLength + 2 + 4);
        BYTE* pData = (BYTE*)strOut.c_str();
        *(WORD*)pData = sHead; pData += 2;
        *(DWORD*)pData = nLength; pData += 4;
        *(WORD*)pData = sCmd; pData += 2;
        memcpy(pData, sData.c_str(), sData.size()); pData += sData.size();
        *(WORD*)pData = sSum;
        return strOut.c_str();
    }
    ~CPacket() {}
public:
    WORD sHead; // ��ͷ���̶�λ�� 0xFEFF
    DWORD nLength; // ������
    WORD sCmd; // ��������
    std::string sData; // ������
    WORD sSum; // ��У��
    std::string strOut; // ������������
};
#pragma pack(pop)

typedef struct MouseEvent {
    MouseEvent() {
        nAction = 0;
        nButton = -1;
        ptXY.x = 0;
        ptXY.y = 0;
    };
    WORD nAction; // ������ƶ���˫��
    WORD nButton; // ������Ҽ����м�
    POINT ptXY; // ����
}MOUSEEV, * PMOUSEEV;

std::string GetErrInfo(int wsaErrorCode);

class CClientSocket
{
public:
    static CClientSocket* getInstance() {
        if (m_instance == NULL) { // ��̬����û��thisָ�룬�޷�ֱ�ӷ��ʳ�Ա����
            m_instance = new CClientSocket();
        }
        return m_instance;
    }

    // ��ʼ��������
    bool InitServer(const std::string& ip) {
        if (m_sock == INVALID_SOCKET) {
            return false;
        }
        sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        serv_addr.sin_port = htons(9527);
        if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
            AfxMessageBox(_T("IP��ַ����"));
            return false;
        }
        int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
        if (ret == SOCKET_ERROR) {
            AfxMessageBox(_T("���ӷ�����ʧ��"));
            TRACE(_T("���ӷ�����ʧ�ܣ�������룺%d��������Ϣ��%s\n"), WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
            return false;
        }
        return true;
    }

#define BUFFER_SIZE 4096
    // ������ƶ�����
    int DealCommand() {
        if (m_sock == INVALID_SOCKET) {
            return -1;
        }
        char* buffer = new char[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        size_t index = 0;
        while (true) {
            size_t recv_len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
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

    // ��������
    bool Send(const char* pData, int nSize) {
        if (m_sock == INVALID_SOCKET) {
            return false;
        }
        return send(m_sock, (char*)pData, nSize, 0) > 0;
    }
    bool Send(CPacket& packet) {
        if (m_sock == INVALID_SOCKET) {
            return false;
        }
        return send(m_sock, packet.Data(), packet.Size(), 0) > 0;
    }
    // ��ȡ�ļ�·��
    bool GetFilePath(std::string& filePath) {
        if (m_packet.sCmd == 2 || m_packet.sCmd == 3 || m_packet.sCmd == 4) {
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

private:
    CClientSocket& operator=(const CClientSocket&) {}
    CClientSocket(const CClientSocket& ss) {
        m_sock = ss.m_sock;
    }

    CClientSocket() {
        m_sock = INVALID_SOCKET;
        if (InitSocketEnv() == FALSE) {
            MessageBox(NULL, _T("�޷���ʼ��Socket������������������"), _T("Error"), MB_OK | MB_ICONERROR);
            exit(0);
        }
        m_sock = socket(PF_INET, SOCK_STREAM, 0);
    }

    ~CClientSocket() {
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
            CClientSocket::getInstance();
        }
        ~CHelper() {
            CClientSocket::releaseInstance();
        }
    };

    static CHelper m_helper;
    static CClientSocket* m_instance;
    SOCKET m_sock;
    CPacket m_packet;
};

extern CClientSocket server;
