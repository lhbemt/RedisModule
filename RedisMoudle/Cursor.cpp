#include "Cursor.h"
#include "json.h"


CCursor::CCursor(DataRecord** pRecord, int nRecords) : m_pRecord(pRecord), m_nRecords(nRecords), m_nCurrRecord(0)
{
	ParseCurrentRecord();
	m_mapType.clear();
	m_mapType["bool"] = 1;
	m_mapType["char"] = 2;
	m_mapType["int32"] = 3;
	m_mapType["uint32"] = 4;
	m_mapType["int64"] = 5;
	m_mapType["uint64"] = 6;
	m_mapType["charn"] = 7;
	m_mapType["blob"] = 8; // 二进制
}


CCursor::~CCursor()
{
}

bool CCursor::EndOfRecord()
{
	return m_nCurrRecord > m_nRecords - 1;
}

void CCursor::MoveNext()
{
	m_nCurrRecord++;
	if (m_nCurrRecord < m_nRecords)
		ParseCurrentRecord();
}

void CCursor::ParseCurrentRecord()
{
	if (m_pRecord) // 有记录 初始化第一条记录
	{
		m_vectFields.clear();
		m_vectValues.clear();
		if (m_pRecord[m_nCurrRecord])
		{
			if ((m_pRecord[m_nCurrRecord])->pData) // 解析数据
			{
				Json::Reader jsonReader;
				Json::Value  jsonValue;
				jsonReader.parse((const char*)(m_pRecord[m_nCurrRecord])->pData, (const char*)(m_pRecord[m_nCurrRecord])->pData + (m_pRecord[m_nCurrRecord])->nLen, jsonValue);
				Json::Value::Members jsonMembers = jsonValue.getMemberNames();
				for (auto iter = jsonMembers.begin(); iter != jsonMembers.end(); iter++)
				{
					m_vectValues.emplace_back(std::move((jsonValue[*iter]).asString()));
					m_vectFields.emplace_back(std::move(*iter));
				}
			}
		}
	}
}

void CCursor::GetValue(std::string strField, char& cValue)
{
	GetValue<char>(strField, cValue, "char");
}

void CCursor::GetValue(std::string strField, bool& bValue)
{
	GetValue<bool>(strField, bValue, "bool");
}

void CCursor::GetValue(std::string strField, int& nValue)
{
	GetValue<int>(strField, nValue, "int32");
}

void CCursor::GetValue(std::string strField, unsigned int& nValue)
{
	GetValue<unsigned int>(strField, nValue, "uint32");
}

void CCursor::GetValue(std::string strField, long long& i64Value)
{
	GetValue<long long>(strField, i64Value, "int64");
}

void CCursor::GetValue(std::string strField, unsigned long long& u64Value)
{
	GetValue<unsigned long long>(strField, u64Value, "uint64");
}

void CCursor::GetValue(std::string strField, char* pValue, int nLen)
{
	if (!pValue)
		return;
	if (m_vectFields.empty())
		return;
	if (m_vectValues.empty())
		return;

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
		return;
	else
		memcpy(pValue, m_vectValues[nIndex].c_str(), nLen);
}

void CCursor::GetValue(std::string strField, void* pValue, int nLen)
{
	GetValue(strField, (char*)pValue, nLen);
}
