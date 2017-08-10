#ifndef __CREDISCOMMAND__H
#define __CREDISCOMMAND__H
#include "redis/hiredis.h"
#include "comm.h"

class CRedisCommand
{
public:
	CRedisCommand();
	~CRedisCommand();

	virtual bool Execute(redisContext* hDBHandle, RedisCommand redisCommandM, const char* command, ...) = 0; // ÷¥––∏√√¸¡Ó

};

#endif