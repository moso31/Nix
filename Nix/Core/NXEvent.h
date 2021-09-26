#pragma once
#include "NXInstance.h"

struct NXEventArgKey
{
	USHORT VKey;
};

struct NXEventArgMouse
{
	USHORT X, Y;
	USHORT VMouse;
	LONG LastX, LastY;
	USHORT VWheel;
};


template<typename... Args>
class NXEvent
{
	using NXEventCallbackFunc = std::function<void(Args...)>;
public:
	NXEvent() {}
	~NXEvent() {}

	void AddListener(NXEventCallbackFunc callbackFunc)
	{
		m_callbackFuncs.push_back(callbackFunc);
	}

	void Notify(Args... args)
	{
		for (auto callbackFunc : m_callbackFuncs)
		{
			callbackFunc(args...);
		}
	}

protected:
	std::vector<NXEventCallbackFunc> m_callbackFuncs;
};

class NXEventKeyUp : public NXEvent<NXEventArgKey>, public NXInstance<NXEventKeyUp> {};
class NXEventKeyDown : public NXEvent<NXEventArgKey>, public NXInstance<NXEventKeyDown> {};
class NXEventMouseUp : public NXEvent<NXEventArgMouse>, public NXInstance<NXEventMouseUp> {};
class NXEventMouseDown : public NXEvent<NXEventArgMouse>, public NXInstance<NXEventMouseDown> {};
class NXEventMouseMove : public NXEvent<NXEventArgMouse>, public NXInstance<NXEventMouseMove> {};

//class NXEventManager : public NXInstance
//{
//public:
//	NXEventManager() :
//		m_EventKeyUp(new NXEvent<NXEventArgKey>),
//		m_EventKeyDown(new NXEvent<NXEventArgKey>),
//		m_EventMouseUp(new NXEvent<NXEventArgMouse>),
//		m_EventMouseDown(new NXEvent<NXEventArgMouse>),
//		m_EventMouseMove(new NXEvent<NXEventArgMouse>)
//	{
//		m_EventKeyUp->GetInstance();
//		m_EventKeyDown->GetInstance();
//		m_EventMouseUp->GetInstance();
//		m_EventMouseDown->GetInstance();
//		m_EventMouseMove->GetInstance();
//	}
//
//	void Release()
//	{
//		delete m_EventKeyUp->GetInstance();
//		delete m_EventKeyDown->GetInstance();
//		delete m_EventMouseUp->GetInstance();
//		delete m_EventMouseDown->GetInstance();
//		delete m_EventMouseMove->GetInstance();
//	}
//
//	NXEvent<NXEventArgKey>*		m_EventKeyUp;
//	NXEvent<NXEventArgKey>*		m_EventKeyDown;
//	NXEvent<NXEventArgMouse>*	m_EventMouseUp;
//	NXEvent<NXEventArgMouse>*	m_EventMouseDown;
//	NXEvent<NXEventArgMouse>*	m_EventMouseMove;
//};