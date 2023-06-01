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
	// ��ͨ�����¼��ࡣ
	// ��ImGuiռ�����/����ʱ����ֹ���������������������¼���

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
				return; // ImGuiռ�����/���̣�������view���ڵ��µģ������ֹ�����¼���
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
	// ǿ�������¼��ࡣ
	// ��ImGuiռ�����/����ʱ��������������������Ȼ���������¼���

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

// ��ͨ�����¼�
// ��ImGuiռ�����/����ʱ����Щ�¼����ᴥ����
class NXEventKeyUp : public NXEvent<NXEventArgKey>, public NXInstance<NXEventKeyUp> {};
class NXEventKeyDown : public NXEvent<NXEventArgKey>, public NXInstance<NXEventKeyDown> {};
class NXEventMouseUp : public NXEvent<NXEventArgMouse>, public NXInstance<NXEventMouseUp> {};
class NXEventMouseDown : public NXEvent<NXEventArgMouse>, public NXInstance<NXEventMouseDown> {};
class NXEventMouseMove : public NXEvent<NXEventArgMouse>, public NXInstance<NXEventMouseMove> {};

// ǿ�������¼���
// ��ImGuiռ�����/����ʱ����Щ�¼���Ȼ�ᴥ����
class NXEventKeyUpForce : public NXForceEvent<NXEventArgKey>, public NXInstance<NXEventKeyUpForce> {};
class NXEventKeyDownForce : public NXForceEvent<NXEventArgKey>, public NXInstance<NXEventKeyDownForce> {};
class NXEventMouseUpForce : public NXForceEvent<NXEventArgMouse>, public NXInstance<NXEventMouseUpForce> {};
class NXEventMouseDownForce : public NXForceEvent<NXEventArgMouse>, public NXInstance<NXEventMouseDownForce> {};
class NXEventMouseMoveForce : public NXForceEvent<NXEventArgMouse>, public NXInstance<NXEventMouseMoveForce> {};

// �������Ƿ����ӿڣ�"view"������ͣ
extern  bool	g_bGuiOnViewportHover;
