#pragma once
#include "NXListener.h"
#include "NXInstance.h"
#include "NXEventArgs.h"

enum NXEventType
{
	NXEVENT_NONE,
	NXEVENT_KEYDOWN,
	NXEVENT_KEYUP,
	NXEVENT_MOUSEDOWN,
	NXEVENT_MOUSEUP,
	NXEVENT_MOUSEMOVE
};

// 监听器机制中的事件。
class NXEvent
{
public:
	NXEvent() : m_type(NXEVENT_NONE) {}
	virtual ~NXEvent();

	// 为当前事件（比如鼠标移动，键盘按下，按钮点击……）添加一个监听器。
	virtual void AddListener(NXListener* pListener);

	// 推送事件。
	virtual void Notify(const NXEventArgs& eArgs);

	NXEventType GetType() const { return m_type; }
	void SetType(const NXEventType type) { m_type = type; }

	void Release();

protected:
	std::vector<NXListener*> m_listeners;
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