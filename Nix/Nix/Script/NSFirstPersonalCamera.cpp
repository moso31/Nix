#include "NSFirstPersonalCamera.h"

#include "NXCamera.h"
#include "NXEvent.h"
#include "NXInput.h"
#include "NXTimer.h"

#define POSITIVE_X 0
#define POSITIVE_Y 1
#define POSITIVE_Z 2
#define NEGATIVE_X 3
#define NEGATIVE_Y 4
#define NEGATIVE_Z 5

#define SPEED_LOW -1
#define SPEED_MID 0
#define SPEED_HIGH 1

NSFirstPersonalCamera::NSFirstPersonalCamera() :
	m_fMoveSpeed(3.0f),
	m_fSensitivity(0.005f),
	m_bSpeedState(SPEED_MID),
	m_bMoveAble(false),
	m_bLastMoveAble(false)
{
	memset(m_bMoveState, false, sizeof(m_bMoveState));
}

NSFirstPersonalCamera::~NSFirstPersonalCamera()
{
}

void NSFirstPersonalCamera::Update()
{
	if (!m_bMoveAble)
		return;

	auto pCamera = dynamic_cast<NXCamera*>(m_pObject);
	Vector3 pos = pCamera->GetTranslation();
	Vector3 fw = pCamera->GetForward();
	Vector3 right = pCamera->GetRight();
	Vector3 up = fw.Cross(right);

	Vector3 moveCommandV(0.0f);
	if (m_bMoveState[POSITIVE_Z]) moveCommandV += fw;
	if (m_bMoveState[NEGATIVE_Z]) moveCommandV -= fw;
	if (m_bMoveState[POSITIVE_X]) moveCommandV += right;
	if (m_bMoveState[NEGATIVE_X]) moveCommandV -= right;
	if (m_bMoveState[POSITIVE_Y]) moveCommandV += up;
	if (m_bMoveState[NEGATIVE_Y]) moveCommandV -= up;

	float moveSpeed = 13.0f;
	switch (m_bSpeedState)
	{
	case SPEED_LOW: moveSpeed *= 1.0f; break;
	case SPEED_MID: moveSpeed *= 3.0f; break;
	case SPEED_HIGH: moveSpeed *= 9.0f; break;
	}

	auto timeDelta = g_timer->GetTimeDelta() / 1000000.0f;

	Vector3 result = pos + moveCommandV * moveSpeed * timeDelta;
	pCamera->SetTranslation(result);

	pCamera->SetRotation(m_fRotation);
}

void NSFirstPersonalCamera::OnKeyDown(NXEventArgKey eArg)
{
	if (eArg.VKey == 'W') m_bMoveState[POSITIVE_Z] = true;
	if (eArg.VKey == 'S') m_bMoveState[NEGATIVE_Z] = true;
	if (eArg.VKey == 'D') m_bMoveState[POSITIVE_X] = true;
	if (eArg.VKey == 'A') m_bMoveState[NEGATIVE_X] = true;
	if (eArg.VKey == 'Q') m_bMoveState[POSITIVE_Y] = true;
	if (eArg.VKey == 'E') m_bMoveState[NEGATIVE_Y] = true;

	if (eArg.VKey == NXKeyCode::LeftShift) m_bSpeedState = SPEED_HIGH;
	if (eArg.VKey == NXKeyCode::LeftControl) m_bSpeedState = SPEED_LOW;
}

void NSFirstPersonalCamera::OnKeyUp(NXEventArgKey eArg)
{
	if (eArg.VKey == 'W') m_bMoveState[POSITIVE_Z] = false;
	if (eArg.VKey == 'S') m_bMoveState[NEGATIVE_Z] = false;
	if (eArg.VKey == 'D') m_bMoveState[POSITIVE_X] = false;
	if (eArg.VKey == 'A') m_bMoveState[NEGATIVE_X] = false;
	if (eArg.VKey == 'Q') m_bMoveState[POSITIVE_Y] = false;
	if (eArg.VKey == 'E') m_bMoveState[NEGATIVE_Y] = false;

	if (eArg.VKey == NXKeyCode::LeftShift) m_bSpeedState = SPEED_MID;
	if (eArg.VKey == NXKeyCode::LeftControl) m_bSpeedState = SPEED_MID;
}

void NSFirstPersonalCamera::OnMouseDown(NXEventArgMouse eArg)
{
	if (eArg.VMouse & 4)	// 4 = mouse right down
	{
		m_bMoveAble = true;
		m_bLastMoveAble = false;
	}
}

void NSFirstPersonalCamera::OnMouseUp(NXEventArgMouse eArg)
{
	if (eArg.VMouse & 8)	// 8 = mouse right up
	{
		m_bMoveAble = false;
		m_bLastMoveAble = false;
	}
}

void NSFirstPersonalCamera::OnMouseMove(NXEventArgMouse eArg)
{
	if (m_bLastMoveAble && m_bMoveAble)
	{
		auto pCamera = dynamic_cast<NXCamera*>(m_pObject);

		float fYaw = (float)eArg.LastX * m_fSensitivity;
		float fPitch = (float)eArg.LastY * m_fSensitivity;

		//Vector3 vUp = pCamera->GetUp();
		//Vector3 vRight = pCamera->GetRight();

		//Matrix mxOld = Matrix::CreateFromZXY(pCamera->GetRotation());
		//Matrix mxRot = Matrix::CreateFromAxisAngle(vRight, fPitch) * Matrix::CreateFromAxisAngle(vUp, fYaw);
		//Matrix mxNew = mxOld * mxRot;

		//m_fRotation = mxNew.EulerRollPitchYaw();
		//printf("OnMouseMove: %f, %f, %f\n", m_fRotation.x, m_fRotation.y, m_fRotation.z);

		float pitchBorder = XM_PIDIV2 - 0.01f;
		float nextPitch = m_fRotation.x + fPitch;
		m_fRotation.x = Clamp(nextPitch, -pitchBorder, pitchBorder);
		m_fRotation.y += fYaw;
	}

	m_bLastMoveAble = m_bMoveAble;
}
