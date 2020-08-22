#pragma once
#include <memory>

template <typename T>
class NXInstance
{
public:
	NXInstance() {}
	virtual ~NXInstance() {}

	static std::shared_ptr<T> GetInstance()
	{
		if (!s_instance) s_instance = std::make_shared<T>();
		return s_instance;
	}

protected:
	static std::shared_ptr<T> s_instance;
};

template <typename T>
std::shared_ptr<T> NXInstance<T>::s_instance;