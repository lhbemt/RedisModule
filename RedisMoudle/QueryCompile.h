#ifndef __CQUERYCOMPILE__H
#define __CQUERYCOMPILE__H

#include "comm.h"
#include "redis/hiredis.h"

class CQueryCompile // ²éÑ¯Ä£¿é
{
public:
	CQueryCompile();
	~CQueryCompile();

	void SetRedisDB(redisContext* hDB)
	{
		m_hDBLogin = hDB;
	}

	bool Query(QueryTree* pQueryTree, const char* tableName);

	const char* GetErrorMsg()
	{
		return m_strErrorMsg.c_str();
	}

private:
	bool ExecuteRedisCommand(RedisCommand RCommand, std::vector<std::string>& vectValues, char* command, ...);

private:
	redisContext* m_hDBLogin;
	std::string   m_strErrorMsg;
};

#endif