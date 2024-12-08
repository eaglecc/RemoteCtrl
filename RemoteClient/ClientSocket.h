#pragma once
#include <string>

#pragma pack(push)
#pragma pack(1)
// 解析和处理数据包
class CPacket {
public:
    CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
    //打包：封装成包
    CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
    {
        sHead = 0xFEFF;
        nLength = nSize + 4; // 命令(2) + 数据(nSize) + 校验和(2)
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
        TRACE("[客户端 封包] sHead=%d nLength=%d nCmd=%d data=[%s]  sSum=%d\r\n", sHead, nLength, sCmd, sData.c_str(), sSum);
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

    // 构造函数重载，从数据包中解析出各个字段 解析包  拆包
    CPacket(const BYTE* pData, size_t& nSize)
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
            i = i + nLength - 2 - 2;
        }

        //取出校验位 并校验 校验位：由数据段的每个字符的低八位的和组成校验位
        sSum = *(WORD*)(pData + i); i += 2;
        WORD sum = 0;
        for (size_t j = 0; j < sData.size(); j++)
        {
            sum += BYTE(sData[j]) & 0xFF;//只取字符低八位
        }
        //TRACE("[客户端] sHead=%d nLength=%d data=[%s]  sSum=%d  sum = %d\r\n", sHead, nLength, strData.c_str(), sSum, sum);
        if (sum == sSum)
        {
            nSize = i;
            return;
        }
        nSize = 0;
    }

    // 包数据大小
    int Size() {
        return nLength + 2 + 4;
    }

    //将包转为字符串类型
    const char* CPacket::Data()
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

    ~CPacket() {}
public:
    WORD sHead; // 包头，固定位： 0xFEFF
    DWORD nLength; // 包长度
    WORD sCmd; // 控制命令
    std::string sData; // 包数据
    WORD sSum; // 和校验
    std::string strOut; // 整个包的数据
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

std::string GetErrInfo(int wsaErrorCode);

class CClientSocket
{
public:
    static CClientSocket* getInstance() {
        if (m_instance == NULL) { // 静态函数没有this指针，无法直接访问成员变量
            m_instance = new CClientSocket();
        }
        return m_instance;
    }

    // 初始化服务器
    bool InitServer(const std::string& ip, int port) {
        if (m_sock == INVALID_SOCKET) {
            return false;
        }
        sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        serv_addr.sin_port = htons(port);
        if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
            AfxMessageBox(_T("IP地址错误"));
            return false;
        }
        int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
        if (ret == SOCKET_ERROR) {
            AfxMessageBox(_T("连接服务器失败"));
            TRACE(_T("连接服务器失败，错误代码：%d，错误信息：%s\n"), WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
            return false;
        }
        return true;
    }

#define BUFFER_SIZE 4096
    // 处理控制端命令
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
                //memmove(buffer, buffer + recv_len, BUFFER_SIZE - recv_len);
                memmove(buffer, buffer + recv_len, index - recv_len);

                index -= recv_len;
                return m_packet.sCmd;
            }
        }
        return -1;
    }

    // 发送数据
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
        TRACE(_T("[客户端] Client Send 客户端发送数据包的控制命令：%d\n"), packet.sCmd);
        return send(m_sock, packet.Data(), packet.Size(), 0) > 0;
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

private:
    CClientSocket& operator=(const CClientSocket&) {}
    CClientSocket(const CClientSocket& ss) {
        m_sock = ss.m_sock;
    }

    CClientSocket() {
        m_sock = INVALID_SOCKET;
        if (InitSocketEnv() == FALSE) {
            MessageBox(NULL, _T("无法初始化Socket环境，请检查网络设置"), _T("Error"), MB_OK | MB_ICONERROR);
            exit(0);
        }
        m_sock = socket(PF_INET, SOCK_STREAM, 0);
    }

    ~CClientSocket() {
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
