#include "RedisMoudle.h"
#include "RedisClientManage.h"

REDIS_MODULE_API REDISHANDLE GetRedisHandle()
{
	return new(std::nothrow) CRedisClientManage;
}

REDIS_MODULE_API bool Init(REDISHANDLE hHandle, const char* serverIP, int nServerPort)
{
	if (hHandle)
		return ((CRedisClientManage*)hHandle)->Init(serverIP, nServerPort);
	return false;
}

REDIS_MODULE_API bool Execute(REDISHANDLE hHandle, REDISRECORD*hRecord,const char* cmd, int nLen)
{
	if (hHandle)
		return ((CRedisClientManage*)hHandle)->Execute(hRecord, cmd, nLen);
	return false;
}

REDIS_MODULE_API const char* GetErrorMsg(REDISHANDLE hHandle)
{
	if (hHandle)
		return ((CRedisClientManage*)hHandle)->GetErrorMsg();
}

REDIS_MODULE_API void GetValue(REDISHANDLE hHandle, REDISRECORD hRecord, const char* strField, char* pValue, int nLen)
{
	if (hHandle)
		return ((CRedisClientManage*)hHandle)->GetValue(&hRecord, strField, pValue, nLen);
}

REDIS_MODULE_API bool EndOfRecord(REDISHANDLE hHandle, REDISRECORD hRecord)
{
	if (hHandle)
		return ((CRedisClientManage*)hHandle)->EndOfRecord(&hRecord);
}

REDIS_MODULE_API void MoveNext(REDISHANDLE hHandle, REDISRECORD hRecord)
{
	if (hHandle)
		return ((CRedisClientManage*)hHandle)->MoveNext(&hRecord);
}

REDIS_MODULE_API void CloseRecord(REDISHANDLE hHandle, REDISRECORD hRecord)
{
	if (hHandle)
		return ((CRedisClientManage*)hHandle)->CloseRecord(&hRecord);
}

REDIS_MODULE_API void ReleaseRedisHandle(REDISHANDLE hHandle)
{
	if (hHandle)
	{
		delete (CRedisClientManage*)hHandle;
		hHandle = nullptr;
	}
}

REDIS_MODULE_API void OpenRecord(REDISHANDLE hHandle, REDISRECORD hRecord)
{
	if (hHandle)
		return ((CRedisClientManage*)hHandle)->OpenRecord(hRecord);
}