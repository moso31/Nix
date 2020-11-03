#pragma once
#include <memory>

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
	}

protected:
	static T* s_instance;
};

template <typename T>
T* NXInstance<T>::s_instance;