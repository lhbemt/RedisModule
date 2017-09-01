#ifndef __REDISMODLE__H
#define __REDISMODLE__H

#define REDIS_MODULE_API __declspec(dllexport)
#define REDISHANDLE void*
#define REDISRECORD void*

#ifdef __cplusplus
extern "C" {
#endif
	REDIS_MODULE_API REDISHANDLE GetRedisHandle();
	REDIS_MODULE_API bool   Init(REDISHANDLE hHandle, const char* serverIP, int nServerPort);
	REDIS_MODULE_API bool   Execute(REDISHANDLE hHandle, REDISRECORD* hRecord, const char* cmd, int nLen);
	REDIS_MODULE_API void   OpenRecord(REDISHANDLE hHandle, REDISRECORD hRecord);
	REDIS_MODULE_API bool   EndOfRecord(REDISHANDLE hHandle, REDISRECORD hRecord);
	REDIS_MODULE_API void   MoveNext(REDISHANDLE hHandle, REDISRECORD hRecord);
	REDIS_MODULE_API void   GetValue(REDISHANDLE hHandle, REDISRECORD hRecord, const char* strField, char* pValue, int nLen);
	REDIS_MODULE_API void   CloseRecord(REDISHANDLE hHandle, REDISRECORD hRecord);
	REDIS_MODULE_API const char* GetErrorMsg(REDISHANDLE hHandle);
	REDIS_MODULE_API void   ReleaseRedisHandle(REDISHANDLE hHandle);
#ifdef __cplusplus
}
#endif

#endif
