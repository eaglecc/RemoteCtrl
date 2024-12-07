#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;
// m_helper È«¾ÖÎ¨Ò»
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pClient = CClientSocket::getInstance();
