#pragma once
#include <atomic>

class IRefCountable
{
public:
	virtual void IncRef() = 0;
	virtual void DecRef() = 0;
};

class NXRefCountable : public IRefCountable
{
public:
	void IncRef() { ++m_refCount; }
	void DecRef()
	{
		if (--m_refCount == 0)
		{
			delete this;
		}
	}

private:
	std::atomic_int m_refCount = 0;
};
