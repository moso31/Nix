#pragma once
#include "NXScript.h"
#include "NXEvent.h"

#define POSITIVE_X 0
#define POSITIVE_Y 1
#define POSITIVE_Z 2
#define NEGATIVE_X 3
#define NEGATIVE_Y 4
#define NEGATIVE_Z 5

#define SPEED_LOW -1
#define SPEED_MID 0
#define SPEED_HIGH 1

class NSFirstPersonalCamera : public NXScript
{
public:
	NSFirstPersonalCamera();
	~NSFirstPersonalCamera();

	void Update();

	void OnKeyDown(NXEventArgKey eArg);
	void OnKeyUp(NXEventArgKey eArg);
	void OnMouseDown(NXEventArgMouse eArg);
	void OnMouseUp(NXEventArgMouse eArg);
	void OnMouseMove(NXEventArgMouse eArg);

private:
	NXCamera* m_pCamera;
	float m_fMoveSpeed;
	float m_fSensitivity;

	// 检测鼠标右键是否按下。当右键按下时才能启动第一人称视角。
	bool m_bMoveAble;

	// +X, +Y, +Z, -X, -Y, -Z
	bool m_bMoveState[6]; 
	int m_bSpeedState;

	Quaternion m_fRotation;

	TimePoint m_lastTime;
	TimePoint m_currTime;
};
