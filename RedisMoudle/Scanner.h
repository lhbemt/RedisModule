#ifndef __CSCANNER__H
#define __CSCANNER__H

#include "comm.h"
#include <vector>

const int nScanSize = 2048; // 初始化缓冲区大小

class CScanner
{
public:
	CScanner();
	~CScanner();

public:
	void LoadContent(const char* strContent, int nLen); // 加载要扫描的内容
	int Scan();
	const char* GetWord()
	{
		return m_pWord;
	}
	bool ReadCondition(QueryTree*& tree); // 查找条件
private:
	char get();
	void unget();
	bool ReadSet(std::vector<std::string>& vectSetValue, char* pFirst); // 值的集合
private:
	int m_nPos; // 当前扫描到的位置
	int m_nTotal; // 内容的总字节数
	char* m_pBuff; // 要扫描的内容
	char* m_pWord; // 扫描到的单词
};

#endif