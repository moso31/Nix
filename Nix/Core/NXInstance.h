#pragma once
#include <memory>
using namespace std;

template <typename T>
class NXInstance
{
public:
	NXInstance() {}
	virtual ~NXInstance() {}

	static shared_ptr<T> GetInstance()
	{
		if (!s_instance) s_instance = make_shared<T>();
		return s_instance;
	}

protected:
	static shared_ptr<T> s_instance;
};

template <typename T>
shared_ptr<T> NXInstance<T>::s_instance;