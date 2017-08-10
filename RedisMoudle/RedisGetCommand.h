#ifndef __CREDISGETCOMMAND__H
#define __CREDISGETCOMMAND__H
#include "RedisCommand.h"

enum class GET_TYPE
{
	LONGLONG,
	STRING
};

enum class GET_STATUS
{
	STATUS_OK,
	STATUS_ERR,
	STATUS_NOT_EXISTS
};

struct RedisRes
{
	GET_TYPE type;
	GET_STATUS status;
	void* pData;
	int   nLen;
	RedisRes()
	{
		status = GET_STATUS::STATUS_ERR;
		pData = nullptr;
		nLen = 0;
	}
};

class CRedisGetCommand : CRedisCommand
{
public:
	CRedisGetCommand();
	~CRedisGetCommand();

	virtual bool Execute(redisContext* hDBHandle, RedisCommand redisCommandM,const char* command, ...) override;
	RedisRes* GetRedisRes()
	{
		return &res;
	}

private:
	RedisRes res;
};

#endif