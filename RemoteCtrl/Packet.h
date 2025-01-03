#pragma once
#include "framework.h"
#include "pch.h"

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
        IsDirectory = -1;
        HasNext = TRUE;
        memset(szFileName, 0, sizeof(szFileName));
    }
    BOOL IsInvalid; // �Ƿ�����Ч�ļ�
    BOOL IsDirectory; // �Ƿ���Ŀ¼
    BOOL HasNext; // �Ƿ�����һ���ļ�
    char szFileName[256]; // �ļ���

}FILEINFO, * PFILEINFO;