#pragma once
#include "header.h"
#include "NXInstance.h"

class NXInput : public NXInstance<NXInput>
{
public:
	NXInput();
	~NXInput();

	XMINT2 MousePosition();	// 获取鼠标在当前客户端的位置（左上角0,0）
	void RestoreData();

	void Update();
	void UpdateRawInput(LPARAM lParam);
	void UpdateViewPortInput(bool isMouseHovering, const Vector4& vpRect);

	void PrintMouseState();

	void Release();

private:
	// 记录了原生键盘/鼠标状态。
	// 不同的状态对应了不同的事件，以Key为例，
	//		按下 + 激活	= KeyDown
	//		按下 + 未激活 = KeyPressing
	//		弹起 + 激活	= KeyUp
	//		弹起 + 未激活 = 无触发事件
	// Mouse也是同理。
	bool m_keyState[256];		// 键盘是否按下
	bool m_keyActivite[256];	// 键盘是否激活（当前帧被按下/弹起）
	bool m_mouseState[256];		// 鼠标是否按下
	bool m_mouseActivite[256];	// 鼠标是否激活（当前帧被按下/弹起）

	// 记录GUIView的数据
	bool m_isMouseHoverOnView;	// 检测鼠标是否在视口（"view"）上悬停
	Vector4 m_viewPortRect;
};

