#pragma once
#include "NXScript.h"
#include "NXEvent.h"
#include "NXInput.h"
#include "NXTimer.h"
#include "NXCamera.h"

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

	void OnKeyDown(const NXKeyEventArgs& eArg);
	void OnKeyUp(const NXKeyEventArgs& eArg);
	void OnMouseDown(const NXMouseEventArgs& eArg);
	void OnMouseMove(const NXMouseEventArgs& eArg);

private:
	NXCamera* m_pCamera;
	float m_fMoveSpeed;
	float m_fSensitivity;

	// +X, +Y, +Z, -X, -Y, -Z
	bool m_bMoveState[6]; 
	int m_bSpeedState;

	TimePoint m_lastTime;
	TimePoint m_currTime;
};
