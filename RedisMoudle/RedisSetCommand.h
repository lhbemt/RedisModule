#ifndef __CREDISSETCOMMAND__H
#define __CREDISSETCOMMAND__H
#include "RedisCommand.h"

enum class SET_STATUS
{
	STATUS_OK,
	STATUS_ERR,
	STATUS_EXISTS //ÒÑ¾­´æÔÚ
};

class CRedisSetCommand : CRedisCommand
{
public:
	CRedisSetCommand();
	~CRedisSetCommand();

	virtual bool Execute(redisContext* hDBHandle, RedisCommand redisCommandM, const char* command, ...) override;
	SET_STATUS GetStatus() { return m_setStatus; }

private:
	SET_STATUS m_setStatus;
};

#endif