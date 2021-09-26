#pragma once
#include "header.h"
#include "NXInstance.h"

#define MK_LEFT		0
#define MK_MID		1
#define MK_RIGHT	2

enum NXKeyCode
{
	LeftShift = 16,
	LeftControl = 17,
	LeftAlt = 18,
};

class NXInput : public NXInstance<NXInput>
{
public:
	NXInput();
	~NXInput();

	bool KeyDown(int key);
	bool Key(int key);
	bool KeyUp(int key);
	bool MouseDown(int key);
	bool MousePressing(int key);
	bool MouseUp(int key);
	XMINT2 MouseMove();
	XMINT2 MousePosition();	// 获取鼠标在当前客户端的位置（左上角0,0）
	void RestoreData();

	void Update();
	void UpdateRawInput(LPARAM lParam);

	void PrintMouseState();

	void Release();

private:
	bool m_keyState[256];
	bool m_keyActivite[256];

	bool m_mouseState[256];
	bool m_mouseActivite[256];

	XMINT2 m_mouseMove;
};

