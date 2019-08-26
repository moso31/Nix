#pragma once
#include "NXListener.h"
#include "NXInstance.h"

struct NXEventArg
{
	USHORT X, Y;
	union
	{
		USHORT VKey;
		USHORT VMouse;
	};
	LONG LastX, LastY;
	USHORT VWheel;
};

enum NXEventType
{
	NXEVENT_NONE,
	NXEVENT_KEYDOWN,
	NXEVENT_KEYUP,
	NXEVENT_MOUSEDOWN,
	NXEVENT_MOUSEUP,
	NXEVENT_MOUSEMOVE
};

class NXEvent
{
public:
	NXEvent() : m_type(NXEVENT_NONE) {}
	virtual ~NXEvent() {}

	virtual void AddListener(const shared_ptr<NXListener>& pListener);
	void OnNotify(NXEventArg eArg);

	NXEventType GetType() const { return m_type; }
	void SetType(const NXEventType type) { m_type = type; }

protected:
	vector<shared_ptr<NXListener>> m_listeners;
	NXEventType m_type;
};

class NXEventKeyDown : public NXEvent, public NXInstance<NXEventKeyDown>
{
public:
	NXEventKeyDown() { m_type = NXEVENT_KEYDOWN; }
	~NXEventKeyDown() {}
};

class NXEventKeyUp : public NXEvent, public NXInstance<NXEventKeyUp>
{
public:
	NXEventKeyUp() { m_type = NXEVENT_KEYUP; }
	~NXEventKeyUp() {}
};

class NXEventMouseDown : public NXEvent, public NXInstance<NXEventMouseDown>
{
public:
	NXEventMouseDown() { m_type = NXEVENT_MOUSEDOWN; }
	~NXEventMouseDown() {}
};

class NXEventMouseUp : public NXEvent, public NXInstance<NXEventMouseUp>
{
public:
	NXEventMouseUp() { m_type = NXEVENT_MOUSEUP; }
	~NXEventMouseUp() {}
};

class NXEventMouseMove : public NXEvent, public NXInstance<NXEventMouseMove>
{
public:
	NXEventMouseMove() { m_type = NXEVENT_MOUSEMOVE; }
	~NXEventMouseMove() {}
};