#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;
// m_helper ȫ��Ψһ
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pClient = CClientSocket::getInstance();
