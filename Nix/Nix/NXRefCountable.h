#pragma once

#include <atomic>
#include <string>

#if defined(_DEBUG)
#define DEBUG_ACTION(X) X
#else
#define DEBUG_ACTION(X)
#endif

class IRefCountable
{
public:
	virtual void IncRef() = 0;
	virtual void DecRef() = 0;
};

class NXRefCountable : public IRefCountable
{
public:
	NXRefCountable() {}
	virtual ~NXRefCountable() {}

	void IncRef();
	void DecRef();

	inline void SetRefCountDebugName(const std::string& debugName) 
	{
		DEBUG_ACTION(m_refCountDebug = debugName);
	}

private:
	std::atomic_int m_refCount = 0;

private:
	DEBUG_ACTION(std::string m_refCountDebug);
};
