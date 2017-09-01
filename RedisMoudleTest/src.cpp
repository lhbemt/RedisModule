#include "RedisMoudle.h"
#include <iostream>

int main()
{
	REDISHANDLE hHandle = nullptr;
	hHandle = GetRedisHandle();
	bool bRet = false;
	bRet = Init(hHandle, "127.0.0.1", 6379);
	if (!bRet)
	{
		std::cout << GetErrorMsg(hHandle) << std::endl;
		ReleaseRedisHandle(hHandle);
		return 0;
	}

	REDISRECORD hRecord = nullptr;

	bRet = Execute(hHandle, &hRecord, "truncate table student", strlen("truncate table student"));
	if (!bRet)
	{
		std::cout << GetErrorMsg(hHandle) << std::endl;
		ReleaseRedisHandle(hHandle);
		return 0;
	}


	bRet = Execute(hHandle, &hRecord, "insert into student (userid, username) values (1101, 'john smith')", strlen("insert into student (userid, username) values (1101, 'john smith')"));
	if (!bRet)
	{
		std::cout << GetErrorMsg(hHandle) << std::endl;
		ReleaseRedisHandle(hHandle);
		return 0;
	}

	bRet = Execute(hHandle, &hRecord, "insert into student (userid, username) values (1102, 'john smith')", strlen("insert into student (userid, username) values (1101, 'john smith')"));
	if (!bRet)
	{
		std::cout << GetErrorMsg(hHandle) << std::endl;
		ReleaseRedisHandle(hHandle);
		return 0;
	}

	//bRet = Execute(hHandle, hRecord, "delete from student where username = 'john smith'", strlen("delete from student where username = 'john smith'"));
	//if (!bRet)
	//{
	//	std::cout << GetErrorMsg(hHandle) << std::endl;
	//	ReleaseRedisHandle(hHandle);
	//	return 0;
	//}

	bRet = Execute(hHandle, &hRecord, "select * from student where username = 'john smith'", strlen("select * from student where username = 'john smith'"));
	if (!bRet)
	{
		std::cout << GetErrorMsg(hHandle) << std::endl;
		ReleaseRedisHandle(hHandle);
		return 0;
	}
	else
	{
		if (hRecord)
		{
			char szValue[128] = { 0x00 };
			OpenRecord(hHandle, hRecord);
			while (!EndOfRecord(hHandle, hRecord))
			{
				GetValue(hHandle, hRecord, "userid", szValue, sizeof(szValue));
				std::cout << "userid: " << szValue << "\t";
				memset(szValue, 0, sizeof(szValue));
				GetValue(hHandle, hRecord, "username", szValue, sizeof(szValue));
				std::cout << "username: " << szValue << "\t";
				memset(szValue, 0, sizeof(szValue));
				std::cout << std::endl;
				MoveNext(hHandle, hRecord);
			}
			CloseRecord(hHandle, hRecord);
		}
	}

	ReleaseRedisHandle(hHandle);
	std::cin.get();
	return 0;
}