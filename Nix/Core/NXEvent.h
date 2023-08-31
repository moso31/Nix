#pragma once
#include "BaseDefs/Math.h"
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

	// 记录了当前鼠标像素左上角相对ViewPort上的位置，以像素为单位。
	Vector2 ViewPortPos;

	// 记录了当前ViewPort的大小，以像素为单位。
	Vector2 ViewPortSize;
};

// 2023.6.3
// 事件基类
template<typename... Args>
class NXEventBase
{
	using NXEventCallbackFunc = std::function<void(Args...)>;
public:
	NXEventBase() {}
	virtual ~NXEventBase() {}

	void AddListener(NXEventCallbackFunc callbackFunc)
	{
		m_callbackFuncs.emplace_back(std::move(callbackFunc));
	}

	void Notify(Args... args)
	{
		if (!ShouldNotify(args...))
			return;

		for (const auto& callbackFunc : m_callbackFuncs)
		{
			callbackFunc(args...);
		}
	}

protected:
	virtual bool ShouldNotify(Args... args) { return true; }

	std::vector<NXEventCallbackFunc> m_callbackFuncs;
};

template<typename... Args>
class NXEvent : public NXEventBase<Args...> {};

template<typename... Args>
class NXForceEvent : public NXEventBase<Args...> {};

template<typename... Args>
class NXViewportEvent : public NXEventBase<Args...> {};

// 普通输入事件
// 当ImGui占用鼠标/键盘时，这些事件不会触发。
class NXEventKeyUp : public NXEvent<NXEventArgKey>, public NXInstance<NXEventKeyUp> {};
class NXEventKeyDown : public NXEvent<NXEventArgKey>, public NXInstance<NXEventKeyDown> {};
class NXEventMouseUp : public NXEvent<NXEventArgMouse>, public NXInstance<NXEventMouseUp> {};
class NXEventMouseDown : public NXEvent<NXEventArgMouse>, public NXInstance<NXEventMouseDown> {};
class NXEventMouseMove : public NXEvent<NXEventArgMouse>, public NXInstance<NXEventMouseMove> {};

// 强制输入事件：
// 当ImGui占用鼠标/键盘时，这些事件仍然会触发。
class NXEventKeyUpForce : public NXForceEvent<NXEventArgKey>, public NXInstance<NXEventKeyUpForce> {};
class NXEventKeyDownForce : public NXForceEvent<NXEventArgKey>, public NXInstance<NXEventKeyDownForce> {};
class NXEventMouseUpForce : public NXForceEvent<NXEventArgMouse>, public NXInstance<NXEventMouseUpForce> {};
class NXEventMouseDownForce : public NXForceEvent<NXEventArgMouse>, public NXInstance<NXEventMouseDownForce> {};
class NXEventMouseMoveForce : public NXForceEvent<NXEventArgMouse>, public NXInstance<NXEventMouseMoveForce> {};

// 视口输入事件：
// 仅在鼠标悬停在视口上时，这些事件会触发。
class NXEventKeyUpViewport : public NXViewportEvent<NXEventArgKey>, public NXInstance<NXEventKeyUpViewport> {};
class NXEventKeyDownViewport : public NXViewportEvent<NXEventArgKey>, public NXInstance<NXEventKeyDownViewport> {};
class NXEventMouseUpViewport : public NXViewportEvent<NXEventArgMouse>, public NXInstance<NXEventMouseUpViewport> {};
class NXEventMouseDownViewport : public NXViewportEvent<NXEventArgMouse>, public NXInstance<NXEventMouseDownViewport> {};
class NXEventMouseMoveViewport : public NXViewportEvent<NXEventArgMouse>, public NXInstance<NXEventMouseMoveViewport> {};

