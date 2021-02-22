#pragma once
#include "header.h"

// �¼���������
class NXListener
{
public:
	NXListener(NXObject* pObject, const std::function<void(NXEventArgs)>& pFunc);
	virtual ~NXListener() {}

	void SetFunc(const std::function<void(NXEventArgs)>& pFunc);
	std::function<void(NXEventArgs)> GetFunc() const;

	// event notify����ǰlistenerʱ����ִ�д˷�����
	// 2021.2.22 ��ǰʵ�ʽ�ִ��m_pFunc������
	void Update(const NXEventArgs& args);
	void Release();

private:
	NXObject* m_pObject;

	std::function<void(NXEventArgs)> m_pFunc;
};
