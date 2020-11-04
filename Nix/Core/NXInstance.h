#pragma once
#include "Header.h"

template <typename T>
class NXInstance
{
public:
	NXInstance() {}
	//NXInstance& operator=(const NXInstance&) = delete;
	virtual ~NXInstance() {}

	static T& GetInstance()
	{
		static T instance;
		return instance;
	}
};
