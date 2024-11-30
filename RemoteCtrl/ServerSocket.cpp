#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;

CServerSocket* CServerSocket::m_instance = NULL;
// m_helper È«¾ÖÎ¨Ò»
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();