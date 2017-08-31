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
		else if (redisCommandM == RedisCommand::INCR_COMMAND || redisCommandM == RedisCommand::DECR_COMMAND) // 自增 自减
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
		else if (redisCommandM == RedisCommand::SMEMBERS_COMMAND || redisCommandM == RedisCommand::HKEYS_COMMAND || redisCommandM == RedisCommand::HGETALL_COMMAND) // smembers
		{
			if (pReply->type == REDIS_REPLY_ARRAY)
			{
				if (pReply->elements == 0)
					res.status = GET_STATUS::STATUS_OK; // 没有该集合
				else
				{
					int nUsed = 0;
					int nTotal = 0;
					for (int i = 0; i < pReply->elements; ++i)
					{
						redisReply* pElements = pReply->element[i];
						if (pElements->type == REDIS_REPLY_STRING)
						{
							if (nTotal - nUsed < (pElements->len + 1)) // 留一个位置给,
							{
								if (!res.pData)
								{
									res.pData = new char[pElements->len + 1];
									memset(res.pData, 0, pElements->len + 1);
									memcpy(res.pData, pElements->str, pElements->len);
									res.nLen = pElements->len;
									char iDot = ',';
									memcpy((char*)res.pData + res.nLen, &iDot, 1);
									nUsed = pElements->len + 1;
									res.nLen += 1;
									nTotal = pElements->len + 1;
								}
								else
								{
									nTotal = res.nLen * 2;
									char* pExtend = new char[nTotal]; // 可能内存超出 导致new失败
									while (pElements->len > nTotal - nUsed)
									{
										delete[] pExtend;
										pExtend = nullptr;
										nTotal *= 2;
										pExtend = new char[nTotal];
									}
									memset(pExtend, 0, nTotal);
									memcpy(pExtend, res.pData, nUsed);
									delete[] res.pData;
									res.pData = pExtend;
									memcpy(pExtend + nUsed, pElements->str, pElements->len);
									res.nLen = nUsed + pElements->len;
									char iDot = ',';
									memcpy((char*)res.pData + res.nLen, &iDot, 1);
									nUsed += pElements->len + 1;
									res.nLen += 1;
								}
							}
							else
							{
								memcpy((char*)res.pData + nUsed, pElements->str, pElements->len);
								res.nLen = nUsed + pElements->len;
								char iDot = ',';
								memcpy((char*)res.pData + res.nLen, &iDot, 1);
								nUsed += pElements->len + 1;
								res.nLen += 1;
							}
						}
						else
						{
							res.status = GET_STATUS::STATUS_ERR;
							break;
						}
					}
					// 最后一个,用0代替
					((char*)res.pData)[res.nLen - 1] = 0;
					res.status = GET_STATUS::STATUS_OK;
				}
			}
			else
				res.status = GET_STATUS::STATUS_ERR;
		}
		bRet = true;
		freeReplyObject(pReply);
		return bRet;
	}
	return false;
}