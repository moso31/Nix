#pragma once
#include "Header.h"

template<class T>
class NXInstance
{
protected:
	NXInstance() {}
	~NXInstance() {}

public:
	static T* GetInstance()
	{
		static T pInstance;
		return &pInstance;
	}
};
