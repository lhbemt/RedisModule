#ifndef __CCURSOR__H
#define __CCURSOR__H

#include "comm.h"
#include <vector>
#include <iostream>
#include <map>
#include <sstream>

// 游标类 对结果集

class CCursor
{
public:
	CCursor();
	~CCursor();

public:
	bool EndOfRecord(); // 无结果集
	void MoveNext(); // 下一个结果集
	void Init(DataRecord** pRecord);
	void CloseRecord();
	template<class T>
	void GetValue(std::string strField, T& value, std::string type)
	{
		if (m_vectFields.empty())
		{
			memset(&value, 0, sizeof(T));
			return;
		}
		if (m_vectValues.empty())
		{
			memset(&value, 0, sizeof(T));
			return;
		}

		int nIndex = 0;
		bool bFind = false;
		for (int i = 0; i < m_vectFields.size(); ++i)
		{
			if (strField == m_vectFields[i])
			{
				bFind = true;
				nIndex = i;
				break;
			}
		}
		if (!bFind)
		{
			memset(&value, 0, sizeof(T));
			return;
		}
		else
		{
			std::stringstream ss;
			ss.clear();
			ss << m_vectValues[nIndex].c_str();
			switch (m_mapType[type.c_str()])
			{
			case 1: // bool
			{
				bool bValue;
				ss >> bValue;
				memcpy(&value, &bValue, sizeof(bool));
				break;
			}
			case 2: //char
			{
				char cValue;
				ss >> cValue;
				memcpy(&value, &cValue, sizeof(char));
				break;
			}
			case 3: // int
			{
				int nValue;
				ss >> nValue;
				memcpy(&value, &nValue, sizeof(int));
				break;
			}
			case 4: // unsinged int
			{
				unsigned int uValue;
				ss >> uValue;
				memcpy(&value, &uValue, sizeof(unsigned int));
				break;
			}
			case 5: // long long
			{
				long long i64Value;
				ss >> i64Value;
				memcpy(&value, &i64Value, sizeof(long long));
				break;
			}
			case 6: //unsigned long long
			{
				unsigned long long u64Value;
				ss >> u64Value;
				memcpy(&value, &u64Value, sizeof(long long));
				break;
			}
			}
		}
		return;
	}

	void GetValue(std::string strField, char& cValue);
	void GetValue(std::string strField, bool& bValue);
	void GetValue(std::string strField, int&  nValue);
	void GetValue(std::string strField, unsigned int& nValue);
	void GetValue(std::string strField, long long& i64Value);
	void GetValue(std::string strField, unsigned long long& u64Value);
	void GetValue(std::string strField, char* pValue, int nLen);
	void GetValue(std::string strField, void* pValue, int nLen); // 二进制 暂不支持

private:
	void ParseCurrentRecord();

private:
	DataRecord** m_pRecord;
	int          m_nCurrRecord; // 当前记录
	std::vector<std::string> m_vectFields; // 字段
	std::vector<std::string> m_vectValues; // 对应的值
	std::map<std::string, int> m_mapType; // 类型
};

#endif