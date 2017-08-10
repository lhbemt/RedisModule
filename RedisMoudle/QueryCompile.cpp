#include "QueryCompile.h"
#include "RedisGetCommand.h"


CQueryCompile::CQueryCompile()
{
}


CQueryCompile::~CQueryCompile()
{
}

bool CQueryCompile::Query(QueryTree* pQueryTree, const char* tableName)
{
	if (pQueryTree == nullptr) // 表示选择所有 select * from tableName
	{
		std::vector<std::string> vectValues;
		bool bRet = ExecuteRedisCommand(RedisCommand::GET_COMMAND, vectValues, "get %s_table", tableName);
		if (!bRet)
		{
			m_strErrorMsg = "not exists table: " + std::string(tableName);
			return false;
		}
		else
		{

		}
	}
	return true;
}

bool CQueryCompile::ExecuteRedisCommand(RedisCommand RCommand, std::vector<std::string>& vectValues, char* command, ...)
{
	char* buff = new char[2048];
	memset(buff, 0, 2048);
	va_list arg;
	va_start(arg, command);
	vsprintf_s(buff, 2048 , command, arg);

	bool bRet = false;
	if (RCommand == RedisCommand::GET_COMMAND)
	{
		CRedisGetCommand redisGetCommand;
		if (m_hDBLogin)
		{
//			redisGetCommand.Execute(m_hDBLogin, buff);
			//if (redisGetCommand.GetNumOfValues() == 0)
			//	bRet = false;
			//else
			//{
			//	redisGetCommand.GetValues(vectValues);
			//	bRet = true;
			//}
		}
	}

	delete[] buff;
	return bRet;
}