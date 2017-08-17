#ifndef __COMM__H
#define __COMM__H
#include <iostream>
#include <vector>

// 数据结构定义文件

enum class RedisCommand
{
	SET_COMMAND,
	GET_COMMAND,
	SET_SADD,    // 对集合的add命令
	SIS_MEMBER,  // 是否是成员
	INCR_COMMAND, // 自增字段
	HSET_COMMAND, // hash字段命令
	HGET_COMMAND, // hget
	SMEMBERS_COMMAND, // smembers
	HKEYS_COMMAND, //hkeys //获取所有key
	HDEL_COMMAND, //hdel 删除
	DEL_COMMAND, // del 删除key 不管key是string list set 还是hash
	SREM_COMMAND, //srem 
	HGETALL_COMMAND, // hgetall
};

enum class RedisExistKey
{
	IS_TABLE, // 表
	IS_FIELD, // 字段
	IS_INDEX  // 索引
};

enum expression_token // 关键字定义
{
	token_create = 1,
	token_table,
	token_where,
	token_eq, // 等于
	token_ls, // 小于
	token_le, // 小于等于
	token_gt, // 大于
	token_ge, // 大于等于
	token_and,
	token_or,
	token_in,
	token_commit,
	token_delete,
	token_drop,
	token_alter,
	token_update,
	token_values,
	token_select,
	token_from,
	token_set,
	token_all,
	token_lpair, // 左括号
	token_rpair,  // 右括号
	token_idot, // 逗号
	token_word,  // 普通单词
	token_normalend, // 正常结束 用于条件查询区域
	token_index, // 表的索引 用于快速查找
	token_insert, // 插入数据
	token_into, // 插入数据
	token_truncate, // truncate table
};

struct keyword {
	int eToken;
	char             express[20];
};

static struct keyword KeyWords[] = {
	{
		token_create, "create"
	},
	{
		token_table, "table",
	},
	{
		token_where, "where"
	},
	{
		token_eq, "="
	},
	{
		token_ls, "<"
	},
	{
		token_le, "<="
	},
	{
		token_gt, ">"
	},
	{
		token_ge, ">="
	},
	{
		token_and, "and"
	},
	{
		token_or, "or"
    },
	{
		token_in, "in"
	},
	{
		token_commit, "commit"
	},
	{
		token_delete, "delete"
	},
	{
		token_drop, "drop"
	},
	{
		token_alter, "alter"
	},
	{
		token_update, "update"
	},
	{
		token_values, "values"
	},
	{
		token_select, "select"
	},
	{
		token_from, "from"
	},
	{
		token_set, "set"
	},
	{
		token_index, "index"
	},
	{
		token_insert, "insert"
	},
	{
		token_into, "into"
	},
	{
		token_truncate, "truncate"
	}
};

struct QueryCondition // 查询条件
{
	int nToken; // 大于 小于或等于
	char fieldName[128]; // 列名
	char fieldValue[128]; // 值
	std::vector<std::string> vectValueSet;
	QueryCondition()
	{
		nToken = 0;
		memset(fieldName, 0, sizeof(fieldName));
		memset(fieldValue, 0, sizeof(fieldValue));
		vectValueSet.clear();
	}
};

struct QueryNode // 查询节点
{
	int nToken; // and or in等
	struct QueryCondition* condition;
	QueryNode()
	{
		nToken = 0;
		condition = nullptr;
	}
};

struct QueryTree // 查询树
{
	QueryNode* pData;
	QueryTree* lTree;
	QueryTree* rTree;
	QueryTree()
	{
		this->pData = nullptr;
		this->lTree = nullptr;
		this->rTree = nullptr;
	}
};

struct QueryTreeAssist // 辅助
{
	QueryTree* pData;
	int        nLeft; //1left 2right
};

struct IndexValue // 索引的值
{
	int nToken; // 大于 小于或等于
	char fieldValue[128]; // 值
	IndexValue()
	{
		nToken = 0;
		memset(this->fieldValue, 0, sizeof(this->fieldValue));
	}
};

struct DataRecord // 注意得到改记录后要自己释放内存
{
	void* pData; // json格式的记录
	int   nLen;
	DataRecord()
	{
		pData = nullptr;
		nLen = 0;
	}
};

#endif
