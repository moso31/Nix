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
	// 普通输入事件类。
	// 当ImGui占用鼠标/键盘时，禁止这个类的所有派生类推送事件。

	using NXEventCallbackFunc = std::function<void(Args...)>;
public:
	NXEvent() {}
	~NXEvent() {}

	void AddListener(NXEventCallbackFunc callbackFunc)
	{
		m_callbackFuncs.emplace_back(std::move(callbackFunc));
	}

	void Notify(Args... args)
	{
		const ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureMouse || io.WantCaptureKeyboard)
		{
			if (!g_bGuiOnViewportHover)
				return; // ImGui占用鼠标/键盘，除非是view窗口导致的，否则禁止推送事件。
		}

		for (const auto& callbackFunc : m_callbackFuncs)
		{
			callbackFunc(args...);
		}
	}

protected:
	std::vector<NXEventCallbackFunc> m_callbackFuncs;
};

template<typename... Args>
class NXForceEvent
{
	// 强制输入事件类。
	// 当ImGui占用鼠标/键盘时，这个类的所有派生类仍然可以推送事件。

	using NXEventCallbackFunc = std::function<void(Args...)>;
public:
	NXForceEvent() {}
	~NXForceEvent() {}

	void AddListener(NXEventCallbackFunc callbackFunc)
	{
		m_callbackFuncs.emplace_back(std::move(callbackFunc));
	}

	void Notify(Args... args)
	{
		for (const auto& callbackFunc : m_callbackFuncs)
		{
			callbackFunc(args...);
		}
	}

protected:
	std::vector<NXEventCallbackFunc> m_callbackFuncs;
};

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

// 检测鼠标是否在视口（"view"）上悬停
extern  bool	g_bGuiOnViewportHover;
