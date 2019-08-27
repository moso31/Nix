#pragma once
#include "NXScript.h"

#include <chrono>
using namespace chrono;

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

	void SetFPSCamera(const shared_ptr<NXCamera>& pCamera);

	void Update();

	void OnKeyDown(NXEventArg eArg);
	void OnKeyUp(NXEventArg eArg);
	void OnMouseDown(NXEventArg eArg);
	void OnMouseMove(NXEventArg eArg);

private:
	shared_ptr<NXCamera> m_pCamera;
	float m_fMoveSpeed;
	float m_fSensitivity;

	// +X, +Y, +Z, -X, -Y, -Z
	bool m_bMoveState[6]; 
	int m_bSpeedState;

	steady_clock::time_point m_lastTime;
	steady_clock::time_point m_currTime;
};
