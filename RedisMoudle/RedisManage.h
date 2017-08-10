#ifndef __CREDISMANAGE__H
#define __CREDISMANAGE__H

//#ifdef __REDIS_MANAGE_DLL
//#define __REDIS_MANAGE_DLL_API __declspec(dllexport)
//#else
//#define __REDIS_MANAGE_DLL_API __declspec(dllimport)
//#endif

#define __REDIS_MANAGE_DLL_API 

#include "redis/hiredis.h"
#include <iostream>

class  __REDIS_MANAGE_DLL_API CRedisManage
{
public:
	CRedisManage();
	~CRedisManage();

public:
	bool Init(const char* strIP, int nPort); // 初始化

public:
	const char* GetErrorMsg();
	redisContext* GetRedis()
	{
		return m_hDBLogin;
	}

public:
	bool m_bInit; // 初始化
	bool m_bRun;  // 运行中

private:
	redisContext* m_hDBLogin; // 连接redis数据库
	std::string   m_strErrorMsg; // 错误信息
};

#endif