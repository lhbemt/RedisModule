#include "ParseSql.h"
#include "RedisManage.h"
#include <iostream>
#include <string>

CRedisManage g_redisManage;
CParseSql    g_parseSql;
std::string  strPrompt = ">>>";

void RunCmd()
{
	while (1)
	{
		std::string strInput = "";
		std::cout << strPrompt.c_str();
		getline(std::cin, strInput);
		g_parseSql.Parse(strInput.c_str(), strInput.size());
	}
}

int main()
{
	bool bRet = g_redisManage.Init("127.0.0.1", 6379);
	if (!bRet)
	{
		std::cout << "connect redis error" << std::endl;
		return 0;
	}
	g_parseSql.SetRedisConnect(g_redisManage.GetRedis());
	g_parseSql.LoadToken(); // ¼ÓÔØ¹Ø¼ü×Ö
	RunCmd();
}