#pragma once
#include "framework.h"
#include "pch.h"

void Dump(BYTE* pData, size_t nSize);

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
        memset(szFileName, 0, sizeof(szFileName));
        IsDirectory = -1;
        HasNext = FALSE;
    }
    BOOL IsInvalid; // 是否是无效文件
    BOOL IsDirectory; // 是否是目录
    BOOL HasNext; // 是否有下一个文件
    char szFileName[256]; // 文件名

}FILEINFO, * PFILEINFO;


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
        TRACE("[服务端] Accept 等待控制端连接...\n");
        if (m_sock == INVALID_SOCKET) {
            TRACE("服务器套接字无效。\n");
            return false;
        }

        sockaddr_in cli_addr;
        memset(&cli_addr, 0, sizeof(cli_addr));
        int cli_len = sizeof(cli_addr);
        m_cli_sock = accept(m_sock, (sockaddr*)&cli_addr, &cli_len);

        if (m_cli_sock == INVALID_SOCKET) {
            TRACE("接受连接失败，错误码：%d\n", WSAGetLastError());
            return false;
        }

        TRACE("收到了来自%s的连接请求\n   端口号：%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        return true;
    }
#define BUFFER_SIZE 4096
    // 处理客户端命令
    int DealCommand() {
        if (m_cli_sock == INVALID_SOCKET) {
            return -1;
        }
        TRACE("[服务端] DealCommand开始处理客户端 %d的命令\n", m_cli_sock);
        char* buffer = new char[BUFFER_SIZE];
        if (buffer == NULL) {
            TRACE("内存分配失败。\n");
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
            m_packet = CPacket((BYTE*)buffer, recv_len);//recv_len传入：buffer数据长度   recv_len传出：成功解析数据长度
            TRACE("[服务端] 收到包头：%d, 包长度：%d, 控制命令：%d, 内容：%s, 校验和：%d \n", m_packet.sHead, m_packet.nLength, m_packet.sCmd, m_packet.sData.c_str(), m_packet.sSum);

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

    // 发送数据
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
    // 获取文件路径
    bool GetFilePath(std::string& filePath) {
        if (m_packet.sCmd == 2 || m_packet.sCmd == 3 || m_packet.sCmd == 4) {
            filePath = m_packet.sData;
            return true;
        }
        return false;
    }
    // 获取鼠标事件
    bool GetMouseEvent(MOUSEEV& mouseEvent) {
        if (m_packet.sCmd == 5) {
            memcpy(&mouseEvent, m_packet.sData.c_str(), sizeof(MOUSEEV));
            return true;
        }
    }

    CPacket& GetPacket() {
        return m_packet;
    }

    // 关闭客户端连接
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
            MessageBox(NULL, _T("无法初始化Socket环境，请检查网络设置"), _T("Error"), MB_OK | MB_ICONERROR);
            exit(0);
        }
        m_sock = socket(PF_INET, SOCK_STREAM, 0);
        TRACE("服务器套接字：%d\n", m_sock);
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
