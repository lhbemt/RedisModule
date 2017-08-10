#include "RedisGetCommand.h"



CRedisGetCommand::CRedisGetCommand()
{
}


CRedisGetCommand::~CRedisGetCommand()
{
	if (res.pData)
	{
		delete[] res.pData;
		res.pData = nullptr;
	}
}

bool CRedisGetCommand::Execute(redisContext* hDBHandle, RedisCommand redisCommandM, const char* command, ...) //
{
	if (!hDBHandle)
		return false;

	char* buff = new char[2048];
	memset(buff, 0, 2048);
	va_list arg;
	va_start(arg, command);
	vsprintf_s(buff, 2048, command, arg);
	redisReply* pReply = static_cast<redisReply*>(redisCommand(hDBHandle, buff));
	if (pReply)
	{
		bool bRet = false;
		if (redisCommandM == RedisCommand::GET_COMMAND || redisCommandM == RedisCommand::HGET_COMMAND)
		{
			if (pReply->type == REDIS_REPLY_STRING)
			{
				res.type = GET_TYPE::STRING;
				res.status = GET_STATUS::STATUS_OK;
				if (res.nLen < pReply->len)
				{
					delete[] res.pData;
					res.pData = nullptr;
					res.pData = new char[pReply->len + 1];
					memset(res.pData, 0, pReply->len + 1);
					memcpy(res.pData, pReply->str, pReply->len);
					res.nLen = pReply->len + 1;
				}
				else
				{
					memset(res.pData, 0, res.nLen);
					memcpy(res.pData, pReply->str, pReply->len);
					res.nLen = pReply->len + 1;
				}
			}
			else if (pReply->type == REDIS_REPLY_NIL) // 不存在
			{
				res.status = GET_STATUS::STATUS_NOT_EXISTS;
			}
			bRet = true;
		}
		else if (redisCommandM == RedisCommand::SIS_MEMBER) // 是否是成员
		{
			if (pReply->type == REDIS_REPLY_INTEGER)
			{
				res.type = GET_TYPE::LONGLONG;
				if (pReply->integer == 0) // 不是成员
					res.status = GET_STATUS::STATUS_NOT_EXISTS;
				else if (pReply->integer == 1)
					res.status = GET_STATUS::STATUS_OK;
				else res.status = GET_STATUS::STATUS_ERR; // 错误
			}
		}
		else if (redisCommandM == RedisCommand::INCR_COMMAND) // 自增
		{
			if (pReply->type == REDIS_REPLY_INTEGER)
			{
				res.type = GET_TYPE::LONGLONG;
				if (pReply->integer > 0)
				{
					res.status = GET_STATUS::STATUS_OK;
					if (res.nLen < sizeof(long long))
					{
						delete[] res.pData;
						res.pData = nullptr;
						res.pData = new char[sizeof(long long)];
						memset(res.pData, 0, sizeof(long long));
						memcpy(res.pData, &pReply->integer, sizeof(long long));
						res.nLen = sizeof(long long);
					}
					else
					{
						memset(res.pData, 0, res.nLen);
						memcpy(res.pData, &pReply->integer, sizeof(long long));
						res.nLen = sizeof(long long);
					}
				}
				else
					res.status = GET_STATUS::STATUS_ERR;
			}
			else
				res.status = GET_STATUS::STATUS_ERR;
		}


		
		//if (pReply->elements == 0 && !pReply->str) // 执行
		//	bRet = false;
		//else if (pReply->elements == 0 && pReply->str)
		//{
		//	m_vectValues.push_back(pReply->str);
		//	bRet = true;
		//}
		//else if (pReply->elements > 0)
		//{
		//	for (int i = 0; i < pReply->elements; i++)
		//	{
		//		redisReply* pElements = pReply->element[i];
		//		if (!pElements->str) // 没有该字段 false
		//			bRet = false;
		//		else
		//			m_vectValues.push_back(pElements->str);
		//	}
		//}

		bRet = true;
		freeReplyObject(pReply);
		return bRet;
	}
	return false;
}