#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;

CServerSocket* CServerSocket::m_instance = NULL;
// m_helper ȫ��Ψһ
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();