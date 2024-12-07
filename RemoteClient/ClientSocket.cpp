#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;
// m_helper È«¾ÖÎ¨Ò»
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pClient = CClientSocket::getInstance();

std::string GetErrInfo(int wsaErrorCode) {
    std::string ret;
    LPVOID lpMsgBuf = NULL;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        wsaErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0,
        NULL
    );
    ret = (char*)lpMsgBuf;
    LocalFree(lpMsgBuf);
    return ret;
}