#ifndef __CPARSESQL__H
#define __CPARSESQL__H
#include "Scanner.h"
#include "redis/hiredis.h"
#include <iostream>

const int TOKEN_ITEM_SIZE = 120;

class TokensTable
{

private:
	struct HashNode
	{
		keyword token;
		HashNode* pNext;
		HashNode()
		{
			token.eToken = -1;
			memset(token.express, 0, sizeof(token.express));
			pNext = nullptr;
		}
	};

private:
	TokensTable() {}
public:
	~TokensTable()
	{
		for (int i = 0; i < TOKEN_ITEM_SIZE; i++)
		{
			HashNode hashNode = hashTable[i];
			DeleteHashNode(hashNode.pNext);
		}
	}
private:
	void DeleteHashNode(HashNode*& pNode)
	{
		if (!pNode)
			return;
		DeleteHashNode(pNode->pNext);
		delete pNode;
		pNode = nullptr;
	}
public:
	static int AddTokens(keyword token);
	static bool FindToken(const char* pWord, int& nToken);
private:
	static int GetHashCode(const char* pWord, int tablesize);
private:
	static HashNode hashTable[TOKEN_ITEM_SIZE];
};

void AddAllTokens();

class CParseSql // 解析sql语句模块
{
public:
	CParseSql();
	~CParseSql();

public:
	static void LoadToken();
	bool Parse(const char* strContent, int nLen); //解析sql语句
	const char* GetErrorMsg()
	{
		return m_strError.c_str();
	}
	void SetRedisConnect(redisContext* hDB)
	{
		m_hDBLogin = hDB;
	}

private:
	bool CreateTable(); // 创建表
	bool SelectFromTable(); // 选择表
	bool InsertIntoTable(); // 插入数据
	bool CheckKeyExists(const char* pKey, RedisExistKey, bool& bExist,const char* tablename = nullptr); //检测字段是否存在 检测字段时需要tablename
	bool ExecuteRedisCommand(RedisCommand RCommand, void* pGetValue, int& nLen,char* command, ...); // 对返回值不感兴趣则pGetValue为nullptr
	bool SetCommand(RedisCommand RCommand);
	bool DoSelect(std::vector<std::string>& vectFields, QueryTree* pTree, const char* tableName); // 进行查找
private:
	CScanner m_Scanner;
	std::string m_strError;
	redisContext* m_hDBLogin;
};

#endif