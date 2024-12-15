#pragma once
#include "framework.h"
#include "pch.h"

void Dump(BYTE* pData, size_t nSize);

#pragma pack(push)
#pragma pack(1)
//����
//���ã�������������ݴ���
//��ʽ��[0xFEFF | ������ | �������� | ���� | ����λ]
//���ȣ�[2B     |    4B |       2B | data|     2B]
//
//��ƣ������� = ��������� + ���ݳ��� + ����λ����
//����λ = ���ݶ�ÿ���ַ��ĵͰ�λ�ĺ�
//�������� = ����˰����еĲ��� ���� 1�鿴���� 2�鿴�ļ� 1981���԰�
//
//���շ����յ���ʱ�����ݰ�ͷʶ���������а�����ʼλ�ã����ݿ������������Ӧ�Ĳ��������ݼ���λ�жϽ��������Ƿ����
class CPacket {
public:
    WORD sHead; // ��ͷ���̶�λ�� 0xFEFF
    DWORD nLength; // ������
    WORD sCmd; // ��������
    std::string sData; // ������
    WORD sSum; // ��У��
    std::string strOut; // ������������

public:
    CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

    //�������װ�ɰ�
    CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
    {
        sHead = 0xFEFF;
        nLength = (nSize + 4); // ����(2) + ����(nSize) + У���(2)
        sCmd = nCmd;

        if (nSize > 0)//�����ݶ�
        {
            //������ݶ�
            sData.resize(nSize);
            memcpy((void*)sData.c_str(), pData, nSize);
        }
        else//�����ݶ�
        {
            sData.clear();
        }

        //�������λ
        sSum = 0;
        for (size_t j = 0; j < sData.size(); j++)
        {
            sSum += BYTE(sData[j]) & 0xFF;//ֻȡ�ַ��Ͱ�λ
        }
        TRACE("[����� ���] sHead=%d nLength=%d nCmd=%d data=[%s]  sSum=%d\r\n", sHead, nLength, sCmd, sData.c_str(), sSum);
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
    CPacket& operator=(const CPacket& pack)
    {
        if (this != &pack)
        {
            sHead = pack.sHead;
            nLength = pack.nLength;
            sCmd = pack.sCmd;
            sData = pack.sData;
            sSum = pack.sSum;
        }
        return *this;
    }

    // ������  ���
    CPacket(const BYTE* pData, size_t& nSize) : sHead(0), nLength(0), sCmd(0), sSum(0)
    {
        //�� [��ͷ2 ������4 ��������2 ������ ��У��2]
        size_t i = 0;
        //ȡ��ͷλ
        for (; i < nSize; i++)
        {
            if ((*(WORD*)(pData + i)) == 0xFEFF)//�ҵ���ͷ
            {
                sHead = *(WORD*)(pData + i);
                i += 2;
                break;
            }
        }

        if ((i + 4 + 2 + 2) > nSize)//�����ݲ�ȫ ֻ�� [��ͷ ������ �������� ��У��]  û�����ݶ� ����ʧ��
        {
            nSize = 0;
            return;
        }

        //ȡ������λ
        nLength = *(DWORD*)(pData + i); i += 4;
        if (nLength + i > nSize)//��δ��ȫ���յ� nLength+sizeof(��ͷ)+sizeof(������) pData������Խ����
        {
            nSize = 0;
            return;
        }

        //ȡ����������λ
        sCmd = *(WORD*)(pData + i); i += 2;

        //�������ݶ�
        if (nLength > 4)
        {
            sData.resize(nLength - 2 - 2);//nLength - [��������λ����] - [У��λ����]
            memcpy((void*)sData.c_str(), pData + i, nLength - 4);
            i = i + (nLength - 2 - 2);
        }

        //ȡ��У��λ ��У�� У��λ�������ݶε�ÿ���ַ��ĵͰ�λ�ĺ����У��λ
        sSum = *(WORD*)(pData + i); i += 2;
        WORD sum = 0;
        for (size_t j = 0; j < sData.size(); j++)
        {
            sum += BYTE(sData[j]) & 0xFF;//ֻȡ�ַ��Ͱ�λ
        }
        if (sum == sSum || (sData.empty() && sum == 0)) // ����Ϊ�գ�У���Ϊ0ʱ�����ǺϷ���
        {
            nSize = i;
            TRACE("[����� ���] sHead=%d nLength=%d nCmd=%d data=[%s]  sSum=%d\r\n", sHead, nLength, sCmd, sData.c_str(), sSum);
            return;
        }
        nSize = 0;
    }

    // �����ݴ�С
    size_t Size() {
        return nLength + 2 + 4;
    }

    //����תΪ�ַ�������
    const char* Data()
    {
        strOut.resize(nLength + 6);
        BYTE* pData = (BYTE*)strOut.c_str();
        *(WORD*)pData = sHead;
        *(DWORD*)(pData + 2) = nLength;
        *(WORD*)(pData + 2 + 4) = sCmd;
        memcpy(pData + 2 + 4 + 2, sData.c_str(), sData.size());
        *(WORD*)(pData + 2 + 4 + 2 + sData.size()) = sSum;
        return strOut.c_str();
    }
    //const char* Data() {
    //    strOut.resize(nLength + 2 + 4);
    //    BYTE* pData = (BYTE*)strOut.c_str();
    //    *(WORD*)pData = sHead; pData += 2;
    //    *(DWORD*)pData = nLength; pData += 4;
    //    *(WORD*)pData = sCmd; pData += 2;
    //    memcpy(pData, sData.c_str(), sData.size()); pData += sData.size();
    //    *(WORD*)pData = sSum;
    //    return strOut.c_str();
    //}
    ~CPacket() {}

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

typedef struct file_info {
    file_info() {
        IsInvalid = FALSE;
        memset(szFileName, 0, sizeof(szFileName));
        IsDirectory = -1;
        HasNext = FALSE;
    }
    BOOL IsInvalid; // �Ƿ�����Ч�ļ�
    BOOL IsDirectory; // �Ƿ���Ŀ¼
    BOOL HasNext; // �Ƿ�����һ���ļ�
    char szFileName[256]; // �ļ���

}FILEINFO, * PFILEINFO;


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
#define BUFFER_SIZE 4096
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
        size_t index = 0;
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
    bool Send(const char* pData, int nSize) {
        if (m_cli_sock == INVALID_SOCKET) {
            return false;
        }
        return send(m_cli_sock, (char*)pData, nSize, 0) > 0;
    }
    bool Send(CPacket& packet) {
        if (m_cli_sock == INVALID_SOCKET) {
            return false;
        }
        Dump((BYTE*)packet.Data(), packet.Size());

        return send(m_cli_sock, packet.Data(), packet.Size(), 0) > 0;
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

    CPacket& GetPacket() {
        return m_packet;
    }

    // �رտͻ�������
    void CloseClient() {
        closesocket(m_cli_sock);
        m_cli_sock = INVALID_SOCKET;
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

    static CHelper m_helper;
    static CServerSocket* m_instance;
    SOCKET m_sock;
    SOCKET m_cli_sock;
    CPacket m_packet;
};
