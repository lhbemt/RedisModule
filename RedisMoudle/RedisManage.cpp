#include "RedisManage.h"
#include <string>


CRedisManage::CRedisManage()
{
	m_hDBLogin = nullptr;
}


CRedisManage::~CRedisManage()
{
}

bool CRedisManage::Init(const char* strIP, int nPort)
{
	if (!strIP || strIP[0] == 0)
	{
		m_strErrorMsg = "strIP is nullptr or no content";
		return false;
	}

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	m_hDBLogin = redisConnect(strIP, nPort);
	if (m_hDBLogin && m_hDBLogin->err)
	{
		std::string str = "connect redis server error" + std::string(m_hDBLogin->errstr);
		m_strErrorMsg = std::move(str);
		return false;
	}
	m_bInit = true;
	return true;
}

const char* CRedisManage::GetErrorMsg()
{
	return m_strErrorMsg.c_str();
}
