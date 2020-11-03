#pragma once
#include "Header.h"

template <typename T>
class NXInstance
{
public:
	NXInstance() {}
	virtual ~NXInstance() 
	{
	}

	static T* GetInstance()
	{
		if (!s_instance) 
			s_instance = new T();
		return s_instance;
	}

	virtual void Destroy() 
	{
		delete s_instance;
	}

protected:
	static T* s_instance;
};

template <typename T>
T* NXInstance<T>::s_instance;