#pragma once
#include "NXScript.h"
#include "NXEvent.h"

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

	// �������Ҽ��Ƿ��¡����Ҽ�����ʱ����������һ�˳��ӽǡ�
	bool m_bMoveAble;

	// +X, +Y, +Z, -X, -Y, -Z
	bool m_bMoveState[6]; 
	int m_bSpeedState;

	Quaternion m_fRotation;

	TimePoint m_lastTime;
	TimePoint m_currTime;
};
