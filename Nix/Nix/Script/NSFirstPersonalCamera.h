#pragma once
#include "NXScript.h"
#include "NXEvent.h"
#include "NXTimer.h"

class NXCamera;
class NSFirstPersonalCamera : public NXScript
{
public:
	NSFirstPersonalCamera();
	~NSFirstPersonalCamera();

	void Update();

	float GetMoveSpeed() { return m_fMoveSpeed; }
	float GetSensitivity() { return m_fSensitivity; }
	void SetMoveSpeed(const float value) { m_fMoveSpeed = value; }
	void SetSensitivity(const float value) { m_fSensitivity = value; }

	void OnKeyDown(const NXEventArgKey& eArg);
	void OnKeyUp(const NXEventArgKey& eArg);
	void OnMouseDown(const NXEventArgMouse& eArg);
	void OnMouseUp(const NXEventArgMouse& eArg);
	void OnMouseMove(const NXEventArgMouse& eArg);

private:
	NXCamera* m_pCamera;
	float m_fMoveSpeed;
	float m_fSensitivity;

	// �������Ҽ��Ƿ��¡����Ҽ�����ʱ����������һ�˳��ӽǡ�
	bool m_bMoveAble;
	bool m_bLastMoveAble;

	// +X, +Y, +Z, -X, -Y, -Z
	bool m_bMoveState[6]; 
	int m_bSpeedState;

	Vector3 m_fRotation;

	TimePoint m_lastTime;
	TimePoint m_currTime;
};
