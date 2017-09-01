#include "ParseSql.h"
#include <cstring>
#include "comm.h"
#include <vector>
#include <algorithm>
#include "RedisSetCommand.h"
#include "json.h"
#include "RedisGetCommand.h"
#include <queue>
#include <sstream>
#include "Cursor.h"

TokensTable::HashNode TokensTable::hashTable[TOKEN_ITEM_SIZE];

int TokensTable::AddTokens(keyword token)
{
	int nHashCode = GetHashCode(token.express, TOKEN_ITEM_SIZE);
	if (hashTable[nHashCode].token.eToken == -1)
		hashTable[nHashCode].token = token;
	else
	{
		if (hashTable[nHashCode].token.eToken == token.eToken)
			return token.eToken;
		else
		{
			HashNode*& pNode = hashTable[nHashCode].pNext;
			while (pNode)
			{
				if (pNode->token.eToken == token.eToken)
					return token.eToken;
				pNode = pNode->pNext;
			}
			pNode = new HashNode;
			pNode->token = token;
		}
	}
	return 0;
}

int TokensTable::GetHashCode(const char* pWord, int tablesize)
{
	int hashval = 0;
	const char* szBegin = pWord;
	while (*szBegin)
	{
		hashval = 37 * hashval + *szBegin;
		hashval %= tablesize;
		szBegin++;
	}
	if (hashval < 0)
		hashval += tablesize;
	return hashval;
}

bool TokensTable::FindToken(const char* pWord, int& nToken)
{
	nToken = 0;
	int nHashCode = GetHashCode(pWord, TOKEN_ITEM_SIZE);
	HashNode hashNode = hashTable[nHashCode];
	if (strcmp(hashNode.token.express, pWord) == 0)
	{
		nToken = hashNode.token.eToken;
		return true;
	}
	else
	{
		HashNode* pNode = hashNode.pNext;
		while (pNode)
		{
			if (strcmp(pNode->token.express, pWord) == 0)
			{
				nToken = pNode->token.eToken;
				return true;
			}
			pNode = pNode->pNext;
		}
	}
	return false;
}

void AddAllTokens()
{
	for (int i = 0; i < sizeof(KeyWords) / sizeof(KeyWords[0]); i++)
		TokensTable::AddTokens(KeyWords[i]);
}

CParseSql::CParseSql()
{
}


CParseSql::~CParseSql()
{
}

void CParseSql::LoadToken() // 加载关键字
{
	AddAllTokens();
}

bool CParseSql::Parse(void** hRecord, const char* strContent, int nLen)
{
	m_Scanner.LoadContent(strContent, nLen);
	int token = m_Scanner.Scan();
	if (token == 0)
		return false;
	switch (token)
	{
	case token_create: // 创建表
	{
		return CreateTable();
	}
	case token_select: // 选择语句
	{
		return SelectFromTable(hRecord);
	}
	case token_insert: // 插入数据
	{
		return InsertIntoTable();
	}
	case token_truncate: // truncate table //未处理回滚
	{
		return TruncateTable();
	}
	case token_drop: // drop table 删除表 //未处理回滚
	{
		return DropTable();
	}
	case token_delete: // delete table // 未处理回滚
	{
		return DeleteTable();
	}
	case token_update: // update table // 未处理回滚
	{
		return UpdateTable();
	}
	default:
		break;
	}
}

bool CParseSql::CreateTable()
{
	char tableName[128] = { 0x00 };
	std::vector<std::string> vectFields, vectIndex;
	std::vector<struct RollBack> vectRollBack; // 操作失败的回滚
	int nWordsToken = m_Scanner.Scan();
	if (nWordsToken != token_table)
	{
		m_strError = "invalid symbol need table";
		return false;
	}
	nWordsToken = m_Scanner.Scan();
	if (nWordsToken != token_word)
	{
		m_strError = "invalid symbol need tablename";
		return false;
	}
	strcpy_s(tableName, sizeof(tableName), m_Scanner.GetWord()); // 获取表名
	nWordsToken = m_Scanner.Scan();
	if (nWordsToken != token_lpair)
	{
		m_strError = "invalid symbol need (";
		return false;
	}
	nWordsToken = m_Scanner.Scan();
	while (nWordsToken != token_rpair)
	{
		if (nWordsToken == token_word) // 列名
			vectFields.push_back(m_Scanner.GetWord()); // 列名
		else if (nWordsToken == token_index)
		{
			nWordsToken = m_Scanner.Scan();
			if (nWordsToken == token_word)
				vectIndex.push_back(m_Scanner.GetWord()); // 索引名
			else
			{
				m_strError = "invalid sysmbol, need normal fields";
				return false;
			}
		}
		nWordsToken = m_Scanner.Scan();
	}

	// 检查索引是否是字段里面的
	bool bFind = true;
	for (auto& iterIndex : vectIndex)
	{
		auto iter = std::find_if(vectIndex.begin(), vectIndex.end(), [iterIndex](std::string fieldName) { return iterIndex == fieldName; });
		if (iter == vectIndex.end())
		{
			bFind = false;
			m_strError = "not all index filed in fields";
			return false;
		}
	}

	// 创建表
	bool bRet = false;
	int n;
	bRet = ExecuteRedisCommand(RedisCommand::SET_COMMAND, nullptr, n, "set %s_table %s", tableName, tableName); // 设置表名
	struct RollBack roll;
	char szRollBack[128] = { 0x00 };
	sprintf_s(szRollBack, sizeof(szRollBack), "del %s_table", tableName);
	roll.cmd = RedisCommand::DEL_COMMAND;
	roll.strDes = szRollBack;
	vectRollBack.emplace_back(std::move(roll));
	if (!bRet)
	{
		m_strError = "set table error";
		BackUp(vectRollBack);
		return false;
	}
	// 创建表的记录自增值
	bRet = ExecuteRedisCommand(RedisCommand::SET_COMMAND, nullptr, n, "set %s_table_id %d", tableName, 0); // 从0开始
	memset(szRollBack, 0, sizeof(szRollBack));
	sprintf_s(szRollBack, sizeof(szRollBack), "del %s_table_id", tableName);
	roll.cmd = RedisCommand::DEL_COMMAND;
	roll.strDes = szRollBack;
	vectRollBack.emplace_back(std::move(roll));
	if (!bRet)
	{
		m_strError = "set table record id error";
		BackUp(vectRollBack);
		return false;
	}
	// 设置列名
	for (auto& field : vectFields)
	{
		bRet = ExecuteRedisCommand(RedisCommand::SET_SADD, nullptr, n, "sadd %s_fields %s", tableName, field.c_str()); // 设置列
		memset(szRollBack, 0, sizeof(szRollBack));
		sprintf_s(szRollBack, sizeof(szRollBack), "srem %s_fields %s", tableName, field.c_str());
		roll.cmd = RedisCommand::SREM_COMMAND;
		roll.strDes = szRollBack;
		vectRollBack.emplace_back(std::move(roll));
		if (!bRet)
		{
			m_strError = "set table field error";
			BackUp(vectRollBack);
			return false;
		}
	}
	// 创建索引
	for (auto& filedIndex : vectIndex)
	{
		bRet = ExecuteRedisCommand(RedisCommand::SET_SADD, nullptr, n, "sadd %s_index %s", tableName, filedIndex.c_str()); // 设置索引
		memset(szRollBack, 0, sizeof(szRollBack));
		sprintf_s(szRollBack, sizeof(szRollBack), "srem %s_index %s", tableName, filedIndex.c_str());
		roll.cmd = RedisCommand::SREM_COMMAND;
		roll.strDes = szRollBack;
		vectRollBack.emplace_back(std::move(roll));
		if (!bRet)
		{
			m_strError = "set table field error";
			BackUp(vectRollBack);
			return false;
		}
	}

	return true;
}

bool CParseSql::SelectFromTable(void** hRecord) // 查询 以结果集的方式返回
{
	QueryTree* tree = nullptr; // 查询树
	std::vector<std::string> vectFields;
	char tableName[128] = { 0x00 };
	int nWordToken = m_Scanner.Scan();
	if (nWordToken == token_all) // select *
	{
		nWordToken = m_Scanner.Scan();
		if (nWordToken != token_from)
		{
			m_strError = "invalid symbol, need from";
			return false;
		}
		else
		{
			nWordToken = m_Scanner.Scan();
			if (nWordToken != token_word)
			{
				m_strError = "invalid symbol, need tablename";
				return false;
			}
			else
			{
				strcpy_s(tableName, sizeof(tableName), m_Scanner.GetWord()); // 表名
				nWordToken = m_Scanner.Scan();
				if (nWordToken == EOF) // 无条件查询
				{

				}
				else // 有条件查询
				{
					if (nWordToken == token_where)
						m_Scanner.ReadCondition(tree); // 查找条件
					else
					{
						m_strError = "invalid symbol, need where";
						return false;
					}

				}
			}
		}
	}
	else // 查询某几个字段 多表查询
	{
		do 
		{
			if (nWordToken == token_word)
			{
				vectFields.push_back(m_Scanner.GetWord());
			}
			else if (nWordToken == token_idot) // 过滤','
			{

			}
			else
			{
				m_strError = "invalid input, need fieldname";
				return false;
			}
			nWordToken = m_Scanner.Scan();
		} while (nWordToken != token_from);

		nWordToken = m_Scanner.Scan();
		if (nWordToken != token_word)
		{
			m_strError = "invalid symbol, need tablename";
			return false;
		}
		else
		{
			strcpy_s(tableName, sizeof(tableName), m_Scanner.GetWord()); // 表名
			nWordToken = m_Scanner.Scan();
			if (nWordToken == EOF) // 无条件查询
			{

			}
			else // 有条件查询
			{
				if (nWordToken == token_where)
					m_Scanner.ReadCondition(tree); // 查找条件
				else
				{
					m_strError = "invalid symbol, need where";
					return false;
				}

			}
		}

	}

	DataRecord** pRecord = nullptr;
	int nRecords = 0;
	std::vector<int> vectNums;
	bool bRet = DoSelect(vectFields, tree, tableName, pRecord, nRecords, vectNums); // need check
	if (!bRet)
	{
		m_strError = "no such records"; // 查询失败
		return false;
	}

	*hRecord = pRecord;

	std::queue<QueryTree*> queueTree;
	if (tree)
	{
		queueTree.push(tree);
		while (!queueTree.empty())
		{
			QueryTree* pTree = queueTree.front();
			queueTree.pop();
			if (pTree->lTree)
				queueTree.push(pTree->lTree);
			if (pTree->rTree)
				queueTree.push(pTree->rTree);
			// delete
			delete pTree->pData->condition;
			delete pTree->pData;
			delete pTree;
		}
	}

	return true;
}

bool CParseSql::InsertIntoTable()
{
	char tableName[128] = { 0x00 };
	int nWordsToken = m_Scanner.Scan();
	if (nWordsToken != token_into)
	{
		m_strError = "invalid sysbmol, need into";
		return false;
	}

	nWordsToken = m_Scanner.Scan();
	if (nWordsToken != token_word)
	{
		m_strError = "invalid input";
		return false;
	}

	strcpy_s(tableName, sizeof(tableName), m_Scanner.GetWord()); // 获取表名
	bool bExists = false;
	bool bRet = CheckKeyExists(tableName, RedisExistKey::IS_TABLE, bExists);
	if (!bExists)
	{
		m_strError = "no such table"; // 没有该表
		return false;
	}

	std::vector<std::string> vectFields; // 字段名
	std::vector<std::string> vectValues; // 值
	std::vector<struct RollBack> vectRollBack;
	nWordsToken = m_Scanner.Scan();
	if (nWordsToken != token_lpair)
	{
		m_strError = "invalid symbol need (";
		return false;
	}
	nWordsToken = m_Scanner.Scan();
	while (nWordsToken != token_rpair)
	{
		if (nWordsToken == token_word) // 列名
			vectFields.push_back(m_Scanner.GetWord()); // 列名
		nWordsToken = m_Scanner.Scan();
	}

	// 检测表字段是否都存在
	for (auto& field : vectFields)
	{
		bool bRet = CheckKeyExists(field.c_str(), RedisExistKey::IS_FIELD, bExists, tableName);
		if (!bExists)
		{
			m_strError = "no such field";
			return false;
		}
	}

	nWordsToken = m_Scanner.Scan();
	if (nWordsToken != token_values)
	{
		m_strError = "invalid symbol, nedd values";
		return false;
	}

	nWordsToken = m_Scanner.Scan();
	if (nWordsToken != token_lpair)
	{
		m_strError = "invalid symbol, nedd (";
		return false;
	}

	nWordsToken = m_Scanner.Scan();
	while (nWordsToken != token_rpair)
	{
		if (nWordsToken == token_word) // 列名
			vectValues.push_back(m_Scanner.GetWord()); // 列名
		nWordsToken = m_Scanner.Scan();
	}

	//判断是否匹配
	if (vectFields.size() != vectValues.size())
	{
		m_strError = "invalid input,fields and values not match";
		return false;
	}

	// 先判断是否有索引 目前仅支持唯一索引 索引字段记录相同时返回错误
	for (int i = 0; i < vectFields.size(); ++i)
	{
		bRet = CheckKeyExists(vectFields[i].c_str(), RedisExistKey::IS_INDEX, bExists, tableName);
		if (bExists) // 是索引 则检查是否已有该索引的记录
		{
			int n = 0;
			bRet = ExecuteRedisCommand(RedisCommand::HGET_COMMAND, nullptr, n, "hget %s_%s_index %s", tableName, vectFields[i].c_str(), vectValues[i].c_str());
			if (bRet)
			{
				m_strError = "already have index record"; // 已经存在该索引记录 插入记录失败
				return false;
			}
		}
	}


	// 打包成json数据
	Json::Value root;
	for (int i = 0; i < vectFields.size(); ++i)
		root[vectFields[i].c_str()] = vectValues[i];
	//std::string jsonStr = root.toStyledString();
	Json::FastWriter writer;
	std::string jsonStr = writer.write(root);

	// 获得记录id
	long long* pID = new long long;
	int n;
	bRet = ExecuteRedisCommand(RedisCommand::INCR_COMMAND, pID, n,"incr %s_table_id", tableName);
	
	struct RollBack roll;
	char szRollBack[128] = { 0x00 };
	sprintf_s(szRollBack, sizeof(szRollBack), "decr %s_table_id", tableName);
	roll.cmd = RedisCommand::DECR_COMMAND;
	roll.strDes = szRollBack;
	vectRollBack.emplace_back(std::move(roll));

	if (!bRet)
	{
		m_strError = "insert error can't increase record";
		BackUp(vectRollBack);
		return false;
	}
	// 获得记录id后 首先更新索引记录 // 唯一索引 后面可新增不唯一索引
	for (int i = 0; i < vectFields.size(); ++i)
	{
		bRet = CheckKeyExists(vectFields[i].c_str(), RedisExistKey::IS_INDEX, bExists, tableName);
		if (bExists) // 是索引 则设置索引记录
		{
			bRet = ExecuteRedisCommand(RedisCommand::HSET_COMMAND, nullptr, n, "hset %s_%s_index %s %I64d", tableName, vectFields[i].c_str(), vectValues[i].c_str(), *pID);
			//ss << "hdel " << tableName << "_" << vectFields[i].c_str() << "_index " << vectValues[i].c_str();
			memset(szRollBack, 0, sizeof(szRollBack));
			sprintf_s(szRollBack, sizeof(szRollBack), "hdel %s_%s_index %s", tableName, vectFields[i].c_str(), vectValues[i].c_str());
			roll.cmd = RedisCommand::HDEL_COMMAND;
			roll.strDes = szRollBack;
			vectRollBack.emplace_back(std::move(roll));
			if (!bRet)
			{
				m_strError = "set index record error";
				BackUp(vectRollBack);
				return false;
			}
		}
	}

	char szRecord[20] = { 0x00 };
	sprintf_s(szRecord, 20, "%I64d", *pID);

	char szBuff[2048] = { 0x00 };
	sprintf_s(szBuff, 2048, "hset %s_record %s %s", tableName, szRecord, jsonStr.c_str());

	// 插入记录
	bRet = ExecuteRedisCommand(RedisCommand::HSET_COMMAND, nullptr, n, "%s", szBuff);
	//ss << "hdel " << tableName << "_record " << szRecord;
	memset(szRollBack, 0, sizeof(szRollBack));
	roll.cmd = RedisCommand::HDEL_COMMAND;
	sprintf_s(szRollBack, "hdel %s_record %s", tableName, szRecord);
	roll.strDes = szRollBack;
	vectRollBack.emplace_back(std::move(roll));
	if (!bRet)
	{
		m_strError = "insert record error";
		BackUp(vectRollBack);
		return false;
	}
	delete pID;
	return true;
}

bool CParseSql::TruncateTable() // truncate table
{
	int nWordToken = 0;
	char tableName[128] = { 0x00 };
	nWordToken = m_Scanner.Scan();
	if (nWordToken != token_table)
	{
		m_strError = "invalid input, need table";
		return false;
	}
	nWordToken = m_Scanner.Scan();
	if (nWordToken != token_word)
	{
		m_strError = "invalid input, need tablename";
		return false;
	}
	strcpy_s(tableName, 128, m_Scanner.GetWord());
	bool bExists = false;
	bool bRet = CheckKeyExists(tableName, RedisExistKey::IS_TABLE, bExists);
	if (!bExists)
	{
		m_strError = "no such table"; // 没有该表
		return false;
	}
	return TruncateTable(tableName);
}

bool CParseSql::TruncateTable(const char* tableName)
{
	// 先删除索引记录 删除失败需回滚 暂不处理

	//先获取索引字段
	char* pIndexFields = new char[1024 * 1024];
	int nLen = 0;
	std::vector<std::string> vectIndexField;
	memset(pIndexFields, 0, 1024 * 1024);
	bool bRet;
	bRet = ExecuteRedisCommand(RedisCommand::SMEMBERS_COMMAND, pIndexFields, nLen, "smembers %s_index", tableName);
	char* sBegin = (char*)pIndexFields;
	char* pBegin = (char*)pIndexFields;
	while (*pIndexFields)
	{
		if (*pIndexFields == ',')
		{
			char Field[128] = { 0x00 };
			memcpy(Field, sBegin, pIndexFields - sBegin);
			vectIndexField.push_back(Field);
			sBegin = pIndexFields + 1;
		}
		pIndexFields = pIndexFields + 1;
	}
	if (*sBegin)
		vectIndexField.push_back(sBegin);

	pIndexFields = pBegin;
	char* szBuff = new char[1024 * 1024]; // 分配大空间 防止不够
	for (auto& field : vectIndexField) // 获取索引对应的记录
	{
		memset(pIndexFields, 0, 1024 * 1024);
		bRet = ExecuteRedisCommand(RedisCommand::HKEYS_COMMAND, pIndexFields, nLen, "hkeys %s_%s_index", tableName, field.c_str());
		if (bRet) // 删除对应的索引记录
		{
			std::vector<std::string> vectIndexValue;
			char* sBegin = (char*)pIndexFields;
			while (*pIndexFields)
			{
				if (*pIndexFields == ',')
				{
					char Field[128] = { 0x00 };
					memcpy(Field, sBegin, pIndexFields - sBegin);
					vectIndexValue.push_back(Field);
					sBegin = pIndexFields + 1;
				}
				pIndexFields = pIndexFields + 1;
			}
			if (*sBegin)
				vectIndexValue.push_back(sBegin);

			if (!vectIndexValue.empty())
			{
				memset(szBuff, 0, 1024 * 1024);
				int nBuffLen = 0;
				nBuffLen = sprintf_s(szBuff, 1024 * 1024, "hdel %s_%s_index ", tableName, field.c_str());
				for (auto& indexValue : vectIndexValue)
					nBuffLen += sprintf_s(szBuff + nBuffLen, 1024 * 1024 - nBuffLen, "%s ", indexValue.c_str());
				//szBuff[nBuffLen - 1] = 0;
				bRet = ExecuteRedisCommand(RedisCommand::HDEL_COMMAND, nullptr, nLen, szBuff);
				if (!bRet)
				{
					m_strError = "delete index record error";
					delete[] szBuff;
					szBuff = nullptr;
					return false;
				}
				delete[] szBuff;
				szBuff = nullptr;
			}
		}

	}
	pIndexFields = pBegin;
	// 删除记录
	memset(pIndexFields, 0, 1024 * 1024);
	szBuff = new char[1024 * 1024];
	memset(szBuff, 0, 1024 * 1024);
	int nBuffLen = 0;
	nBuffLen = sprintf_s(szBuff, 1024 * 1024, "hdel %s_record ", tableName);
	bRet = ExecuteRedisCommand(RedisCommand::HKEYS_COMMAND, pIndexFields, nLen, "hkeys %s_record", tableName);
	if (!bRet)
	{
		m_strError = "delete records error";
		return false;
	}
	else
	{
		char* sBegin = (char*)pIndexFields;
		while (*pIndexFields)
		{
			if (*pIndexFields == ',')
			{
				char Field[128] = { 0x00 };
				memcpy(Field, sBegin, pIndexFields - sBegin);
				nBuffLen += sprintf_s(szBuff + nBuffLen, 1024 * 1024 - nBuffLen, "%s ", Field);
				sBegin = pIndexFields + 1;
			}
			pIndexFields = pIndexFields + 1;
		}
		if (*sBegin)
		{
			nBuffLen += sprintf_s(szBuff + nBuffLen, 1024 * 1024 - nBuffLen, "%s", sBegin);
			//szBuff[nBuffLen - 1] = 0;
			bRet = ExecuteRedisCommand(RedisCommand::HDEL_COMMAND, nullptr, nLen, szBuff);
			if (!bRet)
			{
				m_strError = "delete index record error";
				delete szBuff;
				szBuff = nullptr;
				return false;
			}
		}
	}
	delete[] szBuff;
	szBuff = nullptr;

	delete[] pBegin;

	// 最后把记录值回退到0
	bRet = ExecuteRedisCommand(RedisCommand::SET_COMMAND, nullptr, nLen, "set %s_table_id %d", tableName, 0); // 从0开始
	if (!bRet)
	{
		m_strError = "set table record id error";
		return false;
	}
	return true;
}

bool CParseSql::DropTable()
{
	char tableName[128] = { 0x00 };
	int nWordToken = 0;
	nWordToken = m_Scanner.Scan();
	if (nWordToken != token_table)
	{
		m_strError = "invalid symbol, need table";
		return false;
	}
	nWordToken = m_Scanner.Scan();
	if (nWordToken != token_word)
	{
		m_strError = "invalid symbol, need tablename";
		return false;
	}
	// 检查表是否存在
	strcpy_s(tableName, 128, m_Scanner.GetWord());
	bool bExists = false;
	bool bRet = CheckKeyExists(tableName, RedisExistKey::IS_TABLE, bExists);
	if (!bExists)
	{
		m_strError = "no such table"; // 没有该表
		return false;
	}
	bRet = TruncateTable(tableName);
	if (!bRet)
	{
		m_strError = "truncate table error";
		return false;
	}

	// 删除索引字段
	char* pIndexFields = new char[1024 * 1024];
	int nLen = 0;
	std::vector<std::string> vectIndexField;
	memset(pIndexFields, 0, 1024 * 1024);
	bRet = ExecuteRedisCommand(RedisCommand::SMEMBERS_COMMAND, pIndexFields, nLen, "smembers %s_index", tableName);
	char* sBegin = (char*)pIndexFields;
	char* pBegin = pIndexFields;
	while (*pIndexFields)
	{
		if (*pIndexFields == ',')
		{
			char Field[128] = { 0x00 };
			memcpy(Field, sBegin, pIndexFields - sBegin);
			vectIndexField.push_back(Field);
			sBegin = pIndexFields + 1;
		}
		pIndexFields = pIndexFields + 1;
	}
	if (*sBegin)
		vectIndexField.push_back(sBegin);

	pIndexFields = pBegin;
	for (auto& indexField : vectIndexField)
	{
		bRet = ExecuteRedisCommand(RedisCommand::DEL_COMMAND, nullptr, nLen, "del %s_%s_index", tableName, indexField.c_str());
		if (!bRet)
		{
			m_strError = "delete index field error";
			return false;
		}
	}
	bRet = ExecuteRedisCommand(RedisCommand::DEL_COMMAND, nullptr, nLen, "del %s_index", tableName);
	if (!bRet)
	{
		m_strError = "delete index error";
		return false;
	}

	// 删除字段
	std::vector<std::string> vectFields;
	memset(pIndexFields, 0, 1024 * 1024);
	bRet = ExecuteRedisCommand(RedisCommand::SMEMBERS_COMMAND, pIndexFields, nLen, "smembers %s_fields", tableName);
	sBegin = (char*)pIndexFields;
	while (*pIndexFields)
	{
		if (*pIndexFields == ',')
		{
			char Field[128] = { 0x00 };
			memcpy(Field, sBegin, pIndexFields - sBegin);
			vectFields.push_back(Field);
			sBegin = pIndexFields + 1;
		}
		pIndexFields = pIndexFields + 1;
	}
	if (*sBegin)
		vectFields.push_back(sBegin);

	pIndexFields = pBegin;
	for (auto& Field : vectFields)
	{
		bRet = ExecuteRedisCommand(RedisCommand::SREM_COMMAND, nullptr, nLen, "srem %s_fields %s", tableName, Field.c_str());
		if (!bRet)
		{
			m_strError = "delete field error";
			return false;
		}
	}
	bRet = ExecuteRedisCommand(RedisCommand::DEL_COMMAND, nullptr, nLen, "del %s_fields", tableName);
	if (!bRet)
	{
		m_strError = "delete index error";
		return false;
	}

	// 删除记录id字段
	bRet = ExecuteRedisCommand(RedisCommand::DEL_COMMAND, nullptr, nLen, "del %s_table_id", tableName);
	if (!bRet)
	{
		m_strError = "delete table record id error";
		return false;
	}

	// 删除记录字段
	bRet = ExecuteRedisCommand(RedisCommand::DEL_COMMAND, nullptr, nLen, "del %s_record", tableName);
	if (!bRet)
	{
		m_strError = "delete table record field error";
		return false;
	}

	// 最后删除表
	bRet = ExecuteRedisCommand(RedisCommand::DEL_COMMAND, nullptr, nLen, "del %s_table", tableName);
	if (!bRet)
	{
		m_strError = "delete table error";
		return false;
	}
	return true;
}

bool CParseSql::DeleteTable() // delete
{
	QueryTree* tree = nullptr; // 查询树
	std::vector<std::string> vectFields;
	bool bRet = false;
	DataRecord** pRecord = nullptr;
	int nRecords = 0;
	std::vector<std::string> getFields;
	std::vector<std::string> getValues;
	int nWordToken = m_Scanner.Scan();
	if (nWordToken != token_from)
	{
		m_strError = "invalid input, need from";
		return false;
	}
	char tableName[128] = { 0x00 };
	nWordToken = m_Scanner.Scan();
	if (nWordToken != token_word)
	{
		m_strError = "invalid input, need tablename";
		return false;
	}
	strcpy_s(tableName, 128, m_Scanner.GetWord());

	nWordToken = m_Scanner.Scan();
	if (nWordToken != token_where)
	{
		m_strError = "invalid input, need from";
		return false;
	}
	m_Scanner.ReadCondition(tree);
	std::vector<int> vectNums;
	if (tree)
	{
		bRet = DoSelect(vectFields, tree, tableName, pRecord, nRecords, vectNums); // need check
		if (!bRet)
		{
			m_strError = "select records error";
			return false;
		}
	}

	// 删除记录 vectNums对应的为记录
	// 先删除索引记录
	{
		for (int k = 0; k < vectNums.size(); ++k)
		{
			//先获取索引字段
			char* pIndexFields = new char[1024 * 1024];
			int nLen = 0;
			std::vector<std::string> vectIndexField;
			memset(pIndexFields, 0, 1024 * 1024);
			bool bRet;
			bRet = ExecuteRedisCommand(RedisCommand::SMEMBERS_COMMAND, pIndexFields, nLen, "smembers %s_index", tableName);
			char* sBegin = (char*)pIndexFields;
			char* pBegin = (char*)pIndexFields;
			while (*pIndexFields)
			{
				if (*pIndexFields == ',')
				{
					char Field[128] = { 0x00 };
					memcpy(Field, sBegin, pIndexFields - sBegin);
					vectIndexField.push_back(Field);
					sBegin = pIndexFields + 1;
				}
				pIndexFields = pIndexFields + 1;
			}
			if (*sBegin)
				vectIndexField.push_back(sBegin);
			pIndexFields = pBegin;

			for (auto& field : vectIndexField) // 获取索引对应的记录 为快速找到索引需再建一张value key表，这里暂不处理 使用循环查找
			{
				std::string strRecordIndex = "";
				std::stringstream is;
				is << vectNums[k];
				is >> strRecordIndex;
				bRet = ExecuteRedisCommand(RedisCommand::HGETALL_COMMAND, pIndexFields, nLen, "hgetall %s_%s_index", tableName, field.c_str());
				if (!bRet)
				{
					m_strError = "select indexfield error";
				}
				else
				{
					std::vector<std::string> vectAll;
					char* sBegin = (char*)pIndexFields;
					while (*pIndexFields)
					{
						if (*pIndexFields == ',')
						{
							char Field[128] = { 0x00 };
							memcpy(Field, sBegin, pIndexFields - sBegin);
							vectAll.push_back(Field);
							sBegin = pIndexFields + 1;
						}
						pIndexFields = pIndexFields + 1;
					}
					if (*sBegin)
						vectAll.push_back(sBegin);
					auto iter = std::find_if(vectAll.begin(), vectAll.end(), [strRecordIndex](std::string& str) { return strRecordIndex == str; });
					if (iter != vectAll.end())
					{
						bRet = ExecuteRedisCommand(RedisCommand::HDEL_COMMAND, nullptr, nLen, "hdel %s_%s_index %s", tableName, field.c_str(), (*(--iter)).c_str());
						if (!bRet)
						{
							m_strError = "delete index record error";
						}
					}
					pIndexFields = pBegin;
				}
			}

			delete[] pBegin;
			
			// 直接删除该记录
			int nDelete = vectNums[k];
			bRet = ExecuteRedisCommand(RedisCommand::HDEL_COMMAND, nullptr, nLen, "hdel %s_record %I64d", tableName, (long long)nDelete);
			if (!bRet)
				m_strError = "delete record error";
		}
	}

	//delete
	if (pRecord)
	{

	}

	std::queue<QueryTree*> queueTree;
	if (tree)
	{
		queueTree.push(tree);
		while (!queueTree.empty())
		{
			QueryTree* pTree = queueTree.front();
			queueTree.pop();
			if (pTree->lTree)
				queueTree.push(pTree->lTree);
			if (pTree->rTree)
				queueTree.push(pTree->rTree);
			// delete
			delete pTree->pData->condition;
			delete pTree->pData;
			delete pTree;
		}
	}
	return true;
}

bool CParseSql::UpdateTable()
{
	char tableName[128] = { 0x00 };
	int nWordToken = 0;
	QueryTree* tree = nullptr;
	std::vector<std::string> vectFields;
	std::vector<std::string> vectValue;
	nWordToken = m_Scanner.Scan();
	bool bRet = false;
	if (nWordToken != token_word)
	{
		m_strError = "invalid input, need tablename";
		return false;
	}
	memcpy(tableName, m_Scanner.GetWord(), strlen(m_Scanner.GetWord()));

	nWordToken = m_Scanner.Scan();
	if (nWordToken != token_set)
	{
		m_strError = "invalid input, need set";
		return false;
	}
	
	while (1) // 读取所有要设置的字段
	{
		nWordToken = m_Scanner.Scan();
		if (nWordToken == token_where)
			break;
		if (nWordToken == EOF)
			break;
		if (nWordToken == token_word)
		{
			vectFields.push_back(m_Scanner.GetWord());
			continue;
		}
		if (nWordToken == token_eq)
		{
			nWordToken = m_Scanner.Scan();
			if (nWordToken == token_word)
			{
				vectValue.push_back(m_Scanner.GetWord());
				continue;
			}
		}
		if (nWordToken == token_idot)
			continue;
	}

	DataRecord** pRecord = nullptr;
	int nRecords = 0;
	std::vector<int> recordNums;
	m_Scanner.ReadCondition(tree);
	bRet = DoSelect(vectFields, tree, tableName, pRecord, nRecords, recordNums);
	if (!bRet)
	{
		m_strError = "select records error";
		return false;
	}
	// 更新数据
	if (pRecord)
	{
		std::vector<std::string> getValues;
		std::vector<std::string> getFields;
		for (int i = 0; i < nRecords; ++i)
		{
			Json::Reader jsonReader;
			Json::Value  jsonValue;
			jsonReader.parse((const char*)(pRecord[i]->pData), (const char*)(pRecord[i]->pData) + pRecord[i]->nLen, jsonValue);
			Json::Value::Members jsonMembers = jsonValue.getMemberNames();
			for (auto iter = jsonMembers.begin(); iter != jsonMembers.end(); iter++)
			{
				getValues.emplace_back(std::move((jsonValue[*iter]).asString()));
				getFields.emplace_back(std::move(*iter));
			}

			bool bError = false;
			// 找到对应的字段
			for (int j = 0; j < vectFields.size(); ++j)
			{
				// 先判断要更新的字段是不是索引
				auto iter = std::find_if(getFields.begin(), getFields.end(), [vectFields, j](std::string& fi) { return fi == vectFields[j]; });
				if (iter == getFields.end())
				{
					m_strError = "invalid record";
					bError = true;
					break;
				}
				else
				{
					bool bExists = false;
					CheckKeyExists((vectFields[j]).c_str(), RedisExistKey::IS_INDEX, bExists, tableName); // 判断是否是索引字段
					
					if (bExists)
					{
						// 先判断该字段要更改的记录会不会影响唯一索引
						int n = 0;
						bRet = ExecuteRedisCommand(RedisCommand::HGET_COMMAND, nullptr, n, "hget %s_%s_index %s", tableName, vectFields[j].c_str(), vectValue.at(iter - getFields.begin()).c_str());
						if (bRet)
						{
							m_strError = "already have index record"; // 已经存在该索引记录 插入记录失败
							bError = true;
							break;
						}


						// 先删除索引
						int nLen = 0;
						bool bRet = ExecuteRedisCommand(RedisCommand::HDEL_COMMAND, nullptr, nLen, "hdel %s_%s_index %s", tableName, vectFields[j].c_str(), getValues.at(iter - getFields.begin()).c_str());
						if (!bRet)
						{
							m_strError = "delete index record error";
							bError = true;
							break;
						}
					}
					getValues.at(iter - getFields.begin()) = vectValue[j];
					if (bExists)
					{
						// 插入新的索引
						std::stringstream is;
						is << recordNums[i];
						char szRecordNum[20] = { 0x00 };
						is >> szRecordNum;
						int nLen = 0;
						char szBuff[2048] = { 0x00 };
						sprintf_s(szBuff, 2048, "hset %s_%s_index %s %s", tableName, vectFields[j].c_str(), getValues.at(iter - getFields.begin()).c_str(), szRecordNum);
						bool bRet = ExecuteRedisCommand(RedisCommand::HSET_COMMAND, nullptr, nLen, szBuff);
						if (!bRet)
						{
							m_strError = "delete index record error";
							bError = true;
							break;
						}
					}
				}
			}

			if (!bError)
			{
				// 打包数据
				Json::Value root;
				for (int k = 0; k < getFields.size(); ++k)
					root[getFields[k].c_str()] = getValues[k];
				Json::FastWriter writer;
				std::string jsonStr = writer.write(root);
				int nLen = 0;
				char szRecord[20] = { 0x00 };
				std::stringstream is;
				is << recordNums[i];
				is >> szRecord;

				char szBuff[2048] = { 0x00 };
				sprintf_s(szBuff, 2048, "hset %s_record %s %s", tableName, szRecord, jsonStr.c_str());

				bRet = ExecuteRedisCommand(RedisCommand::HSET_COMMAND, nullptr, nLen, szBuff);
				if (!bRet)
				{
					m_strError = "update record error";
					break;
				}
			}
			else
				break;
			getValues.clear();
			getFields.clear();
		}
	}


	// 释放内存
	if (pRecord)
	{
		for (int i = 0; i < nRecords; ++i)
		{
			if (pRecord[i])
			{
				delete[](pRecord[i])->pData;
				delete (pRecord[i]);
			}
		}
		delete[] pRecord;
	}

	std::queue<QueryTree*> queueTree;
	if (tree)
	{
		queueTree.push(tree);
		while (!queueTree.empty())
		{
			QueryTree* pTree = queueTree.front();
			queueTree.pop();
			if (pTree->lTree)
				queueTree.push(pTree->lTree);
			if (pTree->rTree)
				queueTree.push(pTree->rTree);
			// delete
			delete pTree->pData->condition;
			delete pTree->pData;
			delete pTree;
		}
	}

}

bool CParseSql::CheckKeyExists(const char* pKey, RedisExistKey keyType, bool& bExist,const char* pTableName)
{
	int n;
	if (keyType == RedisExistKey::IS_TABLE) // 检测表是否存在
		bExist = ExecuteRedisCommand(RedisCommand::GET_COMMAND, nullptr, n, "get %s_table", pKey);
	if (keyType == RedisExistKey::IS_FIELD) // 检测字段是否存在
		bExist = ExecuteRedisCommand(RedisCommand::SIS_MEMBER, nullptr, n, "sismember %s_fields %s", pTableName, pKey);
	if (keyType == RedisExistKey::IS_INDEX) // 检测是否为索引字段
		bExist = ExecuteRedisCommand(RedisCommand::SIS_MEMBER, nullptr, n, "sismember %s_index %s", pTableName, pKey);
	return true;
}

bool CParseSql::ExecuteRedisCommand(RedisCommand RCommand, void* pGetValue, int& nLen, char* command, ...)
{
	char* buff = new char[2048];
	memset(buff, 0, 2048);
	va_list arg;
	va_start(arg, command);
	vsprintf_s(buff, 2048, command, arg);
	bool bRet = false;
	if (SetCommand(RCommand))
	{
		CRedisSetCommand reSet;
		bRet = reSet.Execute(m_hDBLogin, RCommand, buff);
		if (SET_STATUS::STATUS_OK != reSet.GetStatus())
			bRet = false;
	}
	else
	{
		CRedisGetCommand reGet;
		bRet = reGet.Execute(m_hDBLogin, RCommand,buff);
		RedisRes* res = reGet.GetRedisRes();
		if (res)
		{
			bRet = (res->status == GET_STATUS::STATUS_OK); // 不存在状态为其他
			if (pGetValue)
			{
				memcpy(pGetValue, res->pData, res->nLen);
				nLen = res->nLen;
			}
		}
	}
	delete[] buff;
	return bRet;
}

bool CParseSql::SetCommand(RedisCommand RCommand)
{
	return RCommand == RedisCommand::SET_COMMAND || RCommand == RedisCommand::SET_SADD || RCommand == RedisCommand::HSET_COMMAND || RCommand == RedisCommand::HDEL_COMMAND
		|| RCommand == RedisCommand::SREM_COMMAND || RCommand == RedisCommand::DEL_COMMAND;
}

bool CParseSql::DoSelect(std::vector<std::string>& vectFields, QueryTree* pTree, const char* tableName, DataRecord**& pRecords, int& nReords, std::vector<int>& vectNums)
{
	// 先检查表是否存在
	bool bExists = false;
	CheckKeyExists(tableName, RedisExistKey::IS_TABLE, bExists);
	if (!bExists)
	{
		m_strError = "no such table";
		return false;
	}
	if (!vectFields.empty()) // 检查所有字段是否正确
	{
		for (auto& filed : vectFields)
		{
			CheckKeyExists(filed.c_str(), RedisExistKey::IS_FIELD, bExists, tableName);
			if (!bExists)
			{
				m_strError = "no such field";
				return false;
			}
		}
	}
	else // 是select * 获取全部表的字段
	{
		void* pFiled = new char[1024];
		memset(pFiled, 0, 1024);
		int nLen = 0;
		char* pBegin = (char*)pFiled;
		bool bRet = ExecuteRedisCommand(RedisCommand::SMEMBERS_COMMAND, pFiled, nLen, "smembers %s_fields", tableName);
		if (!bRet)
		{
			m_strError = "get field error";
			return false;
		}
		else
		{
			char* sBegin = (char*)pFiled;
			char* pBegin = (char*)pFiled;
			while (*((char*)pFiled))
			{
				if (*(char*)pFiled == ',')
				{
					char Field[128] = { 0x00 };
					memcpy(Field, sBegin, (char*)pFiled - sBegin);
					vectFields.push_back(Field);
					sBegin = (char*)pFiled + 1;
				}
				pFiled = (char*)pFiled + 1;
			}
			vectFields.push_back(sBegin);
		}
		delete[] pBegin;
	}
	std::vector<std::string> condiField;
	std::vector<std::string> indexField;
	std::vector<IndexValue> indexValue; // 索引字段对应的值
	std::queue<QueryTree*> queueTree;
	// 先检查条件树是否有索引字段 // 有or的情况下且or的字段不是索引，则按索引查找无意义
	bool bFindOr = false;
	if (pTree)
	{
		queueTree.push(pTree);
		while (!queueTree.empty())
		{
			QueryTree* pFind = queueTree.front();
			queueTree.pop();
			if (pFind->pData)
			{
				std::string str = pFind->pData->condition->fieldName;
				condiField.push_back(str);
				CheckKeyExists(str.c_str(), RedisExistKey::IS_INDEX, bExists, tableName);
				if (bExists)
				{
					IndexValue value;
					value.nToken = pFind->pData->condition->nToken;
					strcpy_s(value.fieldValue, sizeof(pFind->pData->condition->fieldValue), pFind->pData->condition->fieldValue);
					indexField.push_back(str);
					indexValue.push_back(value);
				}
				if (pFind->pData->nToken == token_or)
					bFindOr = true;
			}
			if (pFind->lTree)
				queueTree.push(pFind->lTree);
			if (pFind->rTree)
				queueTree.push(pFind->rTree);
		}
	}

	if (bFindOr) // 有or的情况下 检查条件是否都是索引字段
		if (condiField.size() != indexField.size()) // 条件字段不都是索引 索引无意义 按照顺序比较记录
			indexField.clear();


	// 检查条件字段是否都存在
	for (auto& filed : condiField)
	{
		CheckKeyExists(filed.c_str(), RedisExistKey::IS_FIELD, bExists, tableName);
		if (!bExists)
		{
			m_strError = "no such field";
			return false;
		}
	}

	bool bDoIndex = false;
	void* pValue = new char[1024];
	memset(pValue, 0, 1024);
	int nLen = 0;
	std::vector<std::string> getFields;
	std::vector<std::string> getValues;
	if (!indexField.empty()) // 有索引字段
	{
		// 仅先处理"="的情况
		pRecords = new DataRecord*[indexField.size() + 1]; // 多分配一个空间 用于判断结束
		for (int i = 0; i < indexField.size() + 1; i++)
			pRecords[i] = nullptr;
		int nSatisfy = 0;
		for (int i = 0; i < indexValue.size(); ++i)
		{
			if (indexValue[i].nToken == token_eq) // 相等
			{
				bDoIndex = true;
				bool bRet = ExecuteRedisCommand(RedisCommand::HGET_COMMAND, pValue, nLen, "hget %s_%s_index %s", tableName, indexField[i].c_str(), indexValue[i].fieldValue);
				if (!bRet)
					break;
				else
				{
					char pRecordNum[128] = { 0x00 };
					memcpy(pRecordNum, pValue, 128);
					bRet = ExecuteRedisCommand(RedisCommand::HGET_COMMAND, pValue, nLen, "hget %s_record %s", tableName, (char*)pValue); // 获取记录信息
					if (!bRet)
						break;
					else // 处理记录
					{
						Json::Reader jsonReader;
						Json::Value  jsonValue;
						jsonReader.parse((const char*)pValue, (const char*)pValue + nLen, jsonValue);
						Json::Value::Members jsonMembers = jsonValue.getMemberNames();
						for (auto iter = jsonMembers.begin(); iter != jsonMembers.end(); iter++)
						{
							getValues.emplace_back(std::move((jsonValue[*iter]).asString()));
							getFields.emplace_back(std::move(*iter));
						}
						// 根据条件树查找是否符合为该条记录
						bRet = IsSatisfyRecord(getFields, getValues, pTree);
						if (bRet) // 该条记录符合查找条件
						{
							pRecords[nSatisfy] = new DataRecord;
							pRecords[nSatisfy]->pData = new char[nLen];
							pRecords[nSatisfy]->nLen = nLen;
							memcpy(pRecords[nSatisfy]->pData, pValue, nLen);
							//nReords = nSatisfy + 1;
							nSatisfy++;
							std::stringstream is;
							is << pRecordNum;
							int iRecord = 0;
							is >> iRecord;
							vectNums.push_back(iRecord);
						}
						getValues.clear();
						getFields.clear();
					}
				}
				//break;
			}
		}
		if (nSatisfy == 0)
		{
			nReords = 0;
			delete[] pRecords;
			pRecords = nullptr;
		}
		else
		{
			nReords = nSatisfy;
		}
	}
	if (!bDoIndex) // 没有处理索引 按正常方式查找
	{
		bool bRet = ExecuteRedisCommand(RedisCommand::GET_COMMAND, pValue, nLen, "get %s_table_id", tableName); // 首先获取总记录有多少条
		if (bRet)
		{
			long long records;
			std::stringstream ss;
			ss << (char*)pValue;
			ss >> records;
			pRecords = new DataRecord*[records + 1]; // 多分配一个空间 用于判断结束
			for (int i = 0; i < records + 1; i++)
				pRecords[i] = nullptr;
			int nSatisfy = 0;
			for (long long i = 1; i <= records + 1; ++i) // 记录从1开始
			{
				memset(pValue, 0, 1024);
				bRet = ExecuteRedisCommand(RedisCommand::HGET_COMMAND, pValue, nLen, "hget %s_record %I64d", tableName, i); // 获取记录信息
				if (!bRet)
					continue;
				else // 处理记录
				{
					Json::Reader jsonReader;
					Json::Value  jsonValue;
					jsonReader.parse((const char*)pValue, (const char*)pValue + nLen, jsonValue);
					Json::Value::Members jsonMembers = jsonValue.getMemberNames();
					for (auto iter = jsonMembers.begin(); iter != jsonMembers.end(); iter++)
					{
						getValues.emplace_back(std::move((jsonValue[*iter]).asString()));
						getFields.emplace_back(std::move(*iter));
					}
					// 根据条件树查找是否符合为该条记录
					if (pTree)
						bRet = IsSatisfyRecord(getFields, getValues, pTree);
					else // 无查找条件 全部都符合
						bRet = true;
					if (bRet) // 该条记录符合查找条件
					{
						pRecords[nSatisfy] = new DataRecord;
						pRecords[nSatisfy]->pData = new char[nLen];
						pRecords[nSatisfy]->nLen = nLen;
						memcpy(pRecords[nSatisfy]->pData, pValue, nLen);
						nSatisfy++;
						getFields.clear();
						getValues.clear();
						vectNums.push_back(i);
					}
					else
					{
						getFields.clear();
						getValues.clear();
					}
				}
			}
			if (nSatisfy == 0)
			{
				nReords = 0;
				delete[] pRecords;
				pRecords = nullptr;
			}
			else
			{
				nReords = nSatisfy;
			}
		}
	}
	return true;
}

bool CParseSql::IsSatisfyRecord(std::vector<std::string>& vectFields, std::vector<std::string>& vectValues, QueryTree* pTree) // float类型先不处理
{
	bool bSatisfy = false;
	bool bSatisfyLeft = false;
	bool bSatisfyRight = false;
	if (pTree)
	{
		bool bFind = false;
		int nIndex = 0;
		if (pTree->pData)
		{
			//auto iter = std::find_if(vectFields.begin(), vectFields.end(), [pTree](std::string& field) { return field == std::string(pTree->pData->condition->fieldName); });
			for (int i = 0; i < vectFields.size(); i++)
			{
				if (strcmp(vectFields[i].c_str() , pTree->pData->condition->fieldName) == 0)
				{
					bFind = true;
					nIndex = i;
					break;
				}
			}
			//if (iter == vectFields.end())
			if (!bFind)
				bSatisfy = false;
			else
			{
				//int nIndex = iter - vectFields.begin();
				if (pTree->pData->condition->nToken == token_eq) // 相等
				{
					bSatisfy = (strcmp(vectValues[nIndex].c_str(), pTree->pData->condition->fieldValue) == 0 ? true : false);
				}
			}
		}
	}
	if (pTree->lTree)
		bSatisfyLeft = IsSatisfyRecord(vectFields, vectValues, pTree->lTree);
	if (pTree->rTree)
		bSatisfyRight = IsSatisfyRecord(vectFields, vectValues, pTree->rTree);
	if (pTree->pData->nToken == token_normalend) // 判断节点是and or 或者 正常结束
		return bSatisfy;
	if (pTree->pData->nToken == token_and)
	{
		bool bRet = bSatisfy;
		if (pTree->lTree)
			bRet = bRet && bSatisfyLeft;
		if (pTree->rTree)
			bRet = bRet && bSatisfyRight;
		return bRet;
	}
	if (pTree->pData->nToken == token_or)
	{
		bool bRet = bSatisfy;
		if (pTree->lTree)
			bRet = bRet || bSatisfyLeft;
		if (pTree->rTree)
			bRet = bRet || bSatisfyRight;
		return bRet;
	}
}

bool CParseSql::BackUp(std::vector<struct RollBack>& vectBackUp) //回退
{
	int nLen = 0;
	for (auto& roll : vectBackUp)
		ExecuteRedisCommand(roll.cmd, nullptr, nLen, (char*)roll.strDes.c_str());
	return true;
}