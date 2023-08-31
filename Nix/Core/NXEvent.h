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

	// ��¼�˵�ǰ����������Ͻ����ViewPort�ϵ�λ�ã�������Ϊ��λ��
	Vector2 ViewPortPos;

	// ��¼�˵�ǰViewPort�Ĵ�С��������Ϊ��λ��
	Vector2 ViewPortSize;
};

// 2023.6.3
// �¼�����
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

// �ӿ������¼���
// ���������ͣ���ӿ���ʱ����Щ�¼��ᴥ����
class NXEventKeyUpViewport : public NXViewportEvent<NXEventArgKey>, public NXInstance<NXEventKeyUpViewport> {};
class NXEventKeyDownViewport : public NXViewportEvent<NXEventArgKey>, public NXInstance<NXEventKeyDownViewport> {};
class NXEventMouseUpViewport : public NXViewportEvent<NXEventArgMouse>, public NXInstance<NXEventMouseUpViewport> {};
class NXEventMouseDownViewport : public NXViewportEvent<NXEventArgMouse>, public NXInstance<NXEventMouseDownViewport> {};
class NXEventMouseMoveViewport : public NXViewportEvent<NXEventArgMouse>, public NXInstance<NXEventMouseMoveViewport> {};

