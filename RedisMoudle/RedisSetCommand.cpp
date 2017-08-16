#include "RedisSetCommand.h"
#include <string.h>


CRedisSetCommand::CRedisSetCommand()
{
}


CRedisSetCommand::~CRedisSetCommand()
{
}

bool CRedisSetCommand::Execute(redisContext* hDBHandle, RedisCommand redisCommandM, const char* command, ...) // 设置
{
	if (!hDBHandle)
		return false;

	char* buff = new char[2048];
	memset(buff, 0, 2048);
	va_list arg;
	va_start(arg, command);
	vsprintf_s(buff, 2048,command, arg);
	redisReply* pReply = static_cast<redisReply*>(redisCommand(hDBHandle, buff));
	if (pReply)
	{
		if (redisCommandM == RedisCommand::SET_COMMAND) // set
		{
			if (pReply->type == REDIS_REPLY_STATUS && strcmp(pReply->str, "OK") == 0)
				m_setStatus = SET_STATUS::STATUS_OK;
			else
				m_setStatus = SET_STATUS::STATUS_ERR;
		}
		else if (redisCommandM == RedisCommand::SET_SADD || redisCommandM == RedisCommand::HSET_COMMAND) // sadd hset
		{
			if (pReply->type == REDIS_REPLY_INTEGER && pReply->integer == 0) // 设置失败 已经存在了
				//m_setStatus = SET_STATUS::STATUS_EXISTS;
				m_setStatus = SET_STATUS::STATUS_OK; 
			else if (pReply->type == REDIS_REPLY_INTEGER && pReply->integer == 1) // 设置成功
				m_setStatus = SET_STATUS::STATUS_OK;
			else
				m_setStatus = SET_STATUS::STATUS_ERR; // 错误
				
		}
		else if (redisCommandM == RedisCommand::HDEL_COMMAND || redisCommandM == RedisCommand::SREM_COMMAND || redisCommandM == RedisCommand::DEL_COMMAND)
		{
			if (pReply->type == REDIS_REPLY_INTEGER && pReply->integer >= 0)
				m_setStatus = SET_STATUS::STATUS_OK;
			else
				m_setStatus = SET_STATUS::STATUS_ERR;
		}
		freeReplyObject(pReply);
		return true;
	}
	return false;
}