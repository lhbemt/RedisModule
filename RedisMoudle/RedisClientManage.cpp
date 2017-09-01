#include "RedisClientManage.h"



CRedisClientManage::CRedisClientManage()
{
}


CRedisClientManage::~CRedisClientManage()
{
}

bool CRedisClientManage::Init(const char* redisServerIP, int nPort)
{
	bool bRet = g_redisManage.Init(redisServerIP, nPort);
	if (!bRet)
	{
		m_strErrorMsg = "connect redis server error";
		return false;
	}
	g_parseSql.SetRedisConnect(g_redisManage.GetRedis());
	g_parseSql.LoadToken(); // ¼ÓÔØ¹Ø¼ü×Ö
	return true;
}

bool CRedisClientManage::Execute(void** hRecord, const char* cmd, int nLen)
{
	bool bRet = g_parseSql.Parse(hRecord, cmd, nLen);
	if (!bRet)
	{
		m_strErrorMsg = g_parseSql.GetErrorMsg();
		return false;
	}
	return true;
}

bool CRedisClientManage::EndOfRecord(void* hRecord)
{
	return m_cursor.EndOfRecord();
}
void CRedisClientManage::MoveNext(void* hRecord)
{
	return m_cursor.MoveNext();
}
void CRedisClientManage::GetValue(void* hRecord, const char* strField, char* pValue, int nLen)
{
	return m_cursor.GetValue(strField, pValue, nLen);
}
void CRedisClientManage::CloseRecord(void* hRecord)
{
	return m_cursor.CloseRecord();
}

void CRedisClientManage::OpenRecord(void* hRecord)
{
	m_cursor.Init((DataRecord**)(hRecord));
}