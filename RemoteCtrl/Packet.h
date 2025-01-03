#pragma once
#include "framework.h"
#include "pch.h"

#pragma pack(push)
#pragma pack(1)
//包类
//作用：用在网络的数据传输
//格式：[0xFEFF | 包长度 | 控制命令 | 数据 | 检验位]
//长度：[2B     |    4B |       2B | data|     2B]
//
//设计：包长度 = 控制命令长度 + 数据长度 + 检验位长度
//检验位 = 数据段每个字符的低八位的和
//控制命令 = 代表此包进行的操作 例如 1查看分区 2查看文件 1981测试包
//
//接收方接收到包时，根据包头识别数据流中包的起始位置，根据控制命令进行相应的操作，根据检验位判断接收数据是否错误
class CPacket {
public:
    WORD sHead; // 包头，固定位： 0xFEFF
    DWORD nLength; // 包长度
    WORD sCmd; // 控制命令
    std::string sData; // 包数据
    WORD sSum; // 和校验
    std::string strOut; // 整个包的数据

public:
    CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

    //打包：封装成包
    CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
    {
        sHead = 0xFEFF;
        nLength = (nSize + 4); // 命令(2) + 数据(nSize) + 校验和(2)
        sCmd = nCmd;

        if (nSize > 0)//有数据段
        {
            //打包数据段
            sData.resize(nSize);
            memcpy((void*)sData.c_str(), pData, nSize);
        }
        else//无数据段
        {
            sData.clear();
        }

        //打包检验位
        sSum = 0;
        for (size_t j = 0; j < sData.size(); j++)
        {
            sSum += BYTE(sData[j]) & 0xFF;//只取字符低八位
        }
        TRACE("[服务端 封包] sHead=%d nLength=%d nCmd=%d data=[%s]  sSum=%d\r\n", sHead, nLength, sCmd, sData.c_str(), sSum);
    }


    // 复制构造函数，用于拷贝数据包
    CPacket(const CPacket& packet) {
        sHead = packet.sHead;
        nLength = packet.nLength;
        sCmd = packet.sCmd;
        sData = packet.sData;
        sSum = packet.sSum;
    }
    // 赋值运算符重载
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

    // 解析包  拆包
    CPacket(const BYTE* pData, size_t& nSize) : sHead(0), nLength(0), sCmd(0), sSum(0)
    {
        //包 [包头2 包长度4 控制命令2 包数据 和校验2]
        size_t i = 0;
        //取包头位
        for (; i < nSize; i++)
        {
            if ((*(WORD*)(pData + i)) == 0xFEFF)//找到包头
            {
                sHead = *(WORD*)(pData + i);
                i += 2;
                break;
            }
        }

        if ((i + 4 + 2 + 2) > nSize)//包数据不全 只有 [包头 包长度 控制命令 和校验]  没有数据段 解析失败
        {
            nSize = 0;
            return;
        }

        //取包长度位
        nLength = *(DWORD*)(pData + i); i += 4;
        if (nLength + i > nSize)//包未完全接收到 nLength+sizeof(包头)+sizeof(包长度) pData缓冲区越界了
        {
            nSize = 0;
            return;
        }

        //取出控制命令位
        sCmd = *(WORD*)(pData + i); i += 2;

        //保存数据段
        if (nLength > 4)
        {
            sData.resize(nLength - 2 - 2);//nLength - [控制命令位长度] - [校验位长度]
            memcpy((void*)sData.c_str(), pData + i, nLength - 4);
            i = i + (nLength - 2 - 2);
        }

        //取出校验位 并校验 校验位：由数据段的每个字符的低八位的和组成校验位
        sSum = *(WORD*)(pData + i); i += 2;
        WORD sum = 0;
        for (size_t j = 0; j < sData.size(); j++)
        {
            sum += BYTE(sData[j]) & 0xFF;//只取字符低八位
        }
        if (sum == sSum || (sData.empty() && sum == 0)) // 数据为空，校验和为0时可能是合法包
        {
            nSize = i;
            TRACE("[服务端 拆包] sHead=%d nLength=%d nCmd=%d data=[%s]  sSum=%d\r\n", sHead, nLength, sCmd, sData.c_str(), sSum);
            return;
        }
        nSize = 0;
    }

    // 包数据大小
    size_t Size() {
        return nLength + 2 + 4;
    }

    //将包转为字符串类型
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
    WORD nAction; // 点击、移动、双击
    WORD nButton; // 左键、右键、中键
    POINT ptXY; // 坐标
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
    file_info() {
        IsInvalid = FALSE;
        IsDirectory = -1;
        HasNext = TRUE;
        memset(szFileName, 0, sizeof(szFileName));
    }
    BOOL IsInvalid; // 是否是无效文件
    BOOL IsDirectory; // 是否是目录
    BOOL HasNext; // 是否有下一个文件
    char szFileName[256]; // 文件名

}FILEINFO, * PFILEINFO;