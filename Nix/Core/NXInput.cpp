#include "NXInput.h"
#include "NXEvent.h"

NXInput::NXInput() :
	m_isMouseHoverOnView(false)
{
	::ZeroMemory(m_keyState, sizeof(m_keyState));
	::ZeroMemory(m_keyActivite, sizeof(m_keyActivite));
	::ZeroMemory(m_mouseState, sizeof(m_mouseState));
	::ZeroMemory(m_mouseActivite, sizeof(m_mouseActivite));

	RAWINPUTDEVICE rid[2];
	rid[0].usUsagePage = 0x01;
	rid[0].usUsage = 0x02;
	rid[0].dwFlags = 0; // RIDEV_NOLEGACY;
	rid[0].hwndTarget = 0;

	rid[1].usUsagePage = 0x01;
	rid[1].usUsage = 0x06;
	rid[1].dwFlags = 0; // RIDEV_NOLEGACY;
	rid[1].hwndTarget = 0;

	printf("Raw Input 输入设备注册中...\n");
	if (!RegisterRawInputDevices(rid, 2, sizeof(rid[0])))
	{
		printf("失败：设备注册失败。\n");
		return;
	}
	printf("完成。\n");
}

NXInput::~NXInput()
{
}

XMINT2 NXInput::MousePosition()
{
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(g_hWnd, &p);
	return XMINT2(p.x, p.y);
}

void NXInput::RestoreData()
{
	::ZeroMemory(m_keyActivite, sizeof(m_keyActivite));
	::ZeroMemory(m_mouseActivite, sizeof(m_mouseActivite));
}

void NXInput::UpdateMousePosInfo()
{
	POINT p;
	GetCursorPos(&p);
	m_mouseAbsolutePos = Vector2((float)p.x, (float)p.y);
	ScreenToClient(g_hWnd, &p);
	m_mouseWindowPos = Vector2((float)p.x, (float)p.y);
}

#define REGISTER_MKMOUSESTATE(RI_MOUSE_CODE, MK_CODE, bValue) if ((raw->data.mouse.usButtonFlags & RI_MOUSE_CODE) != 0) m_mouseKey[MK_CODE] = bValue;
void NXInput::UpdateRawInput(LPARAM lParam)
{
	UpdateMousePosInfo();

	UINT dwSize;
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
	LPBYTE lpb = new BYTE[dwSize];
	if (lpb == nullptr)
		return;

	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
		printf("注意：GetRawInputData返回的数据值和预计的长度不匹配！这可能会造成未知错误\n");

	RAWINPUT* raw = (RAWINPUT*)lpb;

	if (raw->header.dwType == RIM_TYPEKEYBOARD)
	{
		//LOGMSG(raw->data.keyboard.MakeCode, raw->data.keyboard.Flags, raw->data.keyboard.Reserved, raw->data.keyboard.ExtraInformation, raw->data.keyboard.Message, raw->data.keyboard.VKey);

		NXEventArgKey eArg;
		eArg.VKey = raw->data.keyboard.VKey;
		bool bIsPressing = (raw->data.keyboard.Flags & 1) == 0;

		m_keyState[eArg.VKey] = bIsPressing;
		m_keyActivite[eArg.VKey] = true;

		if (bIsPressing)
		{
			NXEventKeyDown::GetInstance()->Notify(eArg);
			NXEventKeyDownForce::GetInstance()->Notify(eArg);
		}
		else
		{
			NXEventKeyUp::GetInstance()->Notify(eArg);
			NXEventKeyUpForce::GetInstance()->Notify(eArg);
		}
	}
	else if (raw->header.dwType == RIM_TYPEMOUSE)
	{
		//LOGMSG(raw->data.mouse.usFlags, raw->data.mouse.ulButtons, raw->data.mouse.usButtonFlags, raw->data.mouse.usButtonData, raw->data.mouse.ulRawButtons, raw->data.mouse.lLastX, raw->data.mouse.lLastY, raw->data.mouse.ulExtraInformation);

		NXEventArgMouse eArg;
		XMINT2 cursor = MousePosition();
		eArg.X = cursor.x;
		eArg.Y = cursor.y;
		eArg.VMouse = raw->data.mouse.usButtonFlags;
		eArg.VWheel = raw->data.mouse.usButtonData;
		int iMousePressing = eArg.VMouse;
		int count = 0;

		bool bIsMouseDown = false;
		bool bIsMouseUp = false;
		while (iMousePressing)
		{
			m_mouseState[count] = (iMousePressing & 3) == 1;
			m_mouseActivite[count] = iMousePressing & 3;

			if (m_mouseState[count]) bIsMouseDown = true;
			if (!m_mouseState[count] && m_mouseActivite[count]) bIsMouseUp = true;
			iMousePressing >>= 2;
			count++;
		}

		eArg.LastX = raw->data.mouse.lLastX;
		eArg.LastY = raw->data.mouse.lLastY;

		eArg.ViewPortSize = Vector2(&m_viewPortRect.z) - Vector2(&m_viewPortRect.x);
		eArg.ViewPortPos = {
			m_mouseAbsolutePos.x + 0.5f - m_viewPortRect.x,
			m_mouseAbsolutePos.y + 0.5f - m_viewPortRect.y
		};

		// print test
		//printf("%.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f\n", m_mouseAbsolutePos.x, m_mouseAbsolutePos.y, m_mouseWindowPos.x, m_mouseWindowPos.y, m_viewPortRect.x, m_viewPortRect.y, eArg.ViewPortSize.x, eArg.ViewPortSize.y);

		if (bIsMouseDown)
		{
			NXEventMouseDown::GetInstance()->Notify(eArg);
			NXEventMouseDownForce::GetInstance()->Notify(eArg);

			if (m_isMouseHoverOnView)
				NXEventMouseDownViewport::GetInstance()->Notify(eArg);
		}
		if (bIsMouseUp)
		{
			NXEventMouseUp::GetInstance()->Notify(eArg);
			NXEventMouseUpForce::GetInstance()->Notify(eArg);

			// 2023.6.3 键盘/鼠标弹起类事件无需if约束。
			NXEventMouseUpViewport::GetInstance()->Notify(eArg);
		}
		if (eArg.LastX || eArg.LastY)
		{
			NXEventMouseMove::GetInstance()->Notify(eArg);
			NXEventMouseMoveForce::GetInstance()->Notify(eArg);

			if (m_isMouseHoverOnView)
				NXEventMouseMoveViewport::GetInstance()->Notify(eArg);
		}
	}

	//PrintMouseState();

	SafeDeleteArray(lpb);
}

void NXInput::UpdateViewPortInput(bool isMouseHovering, const Vector4& vpRect)
{
	m_isMouseHoverOnView = isMouseHovering;
	m_viewPortRect = vpRect;
}

void NXInput::PrintMouseState()
{
	bool flag = false;
	for (int i = 0; i < 5; i++)
	{
		if (m_mouseActivite[i])
			flag = true;
	}
	if (!flag)
		return;

	printf("state: ");
	for (int i = 0; i < 5; i++) printf("%d ", m_mouseState[i]);
	printf("activite: ");
	for (int i = 0; i < 5; i++) printf("%d ", m_mouseActivite[i]);
	printf("\n");
}

void NXInput::Release()
{
}