#include "NSFirstPersonalCamera.h"

#include "NXCamera.h"
#include "NXEvent.h"
#include "NXInput.h"
#include "NXTimer.h"

NSFirstPersonalCamera::NSFirstPersonalCamera() :
	m_fMoveSpeed(3.0f),
	m_fSensitivity(0.005f),
	m_bSpeedState(SPEED_MID)
{
	memset(m_bMoveState, false, sizeof(m_bMoveState));
}

NSFirstPersonalCamera::~NSFirstPersonalCamera()
{
}

void NSFirstPersonalCamera::Update()
{
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

	float moveSpeed = 3.0f;
	switch (m_bSpeedState)
	{
	case SPEED_LOW: moveSpeed = 1.0f; break;
	case SPEED_MID: moveSpeed = 3.0f; break;
	case SPEED_HIGH: moveSpeed = 9.0f; break;
	}

	auto timeDelta = g_timer->GetTimeDelta() / 1000000.0f;

	Vector3 result = pos + moveCommandV * moveSpeed * timeDelta;
	pCamera->SetTranslation(result);
}

void NSFirstPersonalCamera::OnKeyDown(NXEventArg eArg)
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

void NSFirstPersonalCamera::OnKeyUp(NXEventArg eArg)
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

void NSFirstPersonalCamera::OnMouseDown(NXEventArg eArg)
{
}

void NSFirstPersonalCamera::OnMouseMove(NXEventArg eArg)
{
	auto pCamera = dynamic_cast<NXCamera*>(m_pObject);

	float fYaw = (float)eArg.LastX * m_fSensitivity;
	float fPitch = (float)eArg.LastY * m_fSensitivity;

	Vector3 vUp = pCamera->GetUp();
	Vector3 vRight = pCamera->GetRight();

	Matrix mxOld = Matrix::CreateFromQuaternion(pCamera->GetRotation());
	Matrix mxRot = Matrix::CreateFromAxisAngle(vRight, fPitch) * Matrix::CreateFromAxisAngle(vUp, fYaw);
	Matrix mxNew = mxOld * mxRot;

	Vector3 ignore;
	Quaternion rotation;
	mxNew.Decompose(ignore, rotation, ignore);
	pCamera->SetRotation(rotation);
}
