#include "NSFirstPersonalCamera.h"

#include "NXCamera.h"
#include "NXEvent.h"
#include "NXInput.h"

NSFirstPersonalCamera::NSFirstPersonalCamera() :
	m_fMoveSpeed(30.0f),
	m_fSensitivity(0.005f),
	m_bSpeedState(SPEED_MID)
{
	memset(m_bMoveState, false, sizeof(m_bMoveState));
}

void NSFirstPersonalCamera::SetFPSCamera(const shared_ptr<NXCamera>& pCamera)
{
	m_pCamera = pCamera;
}

void NSFirstPersonalCamera::Update()
{
	m_currTime = steady_clock::now();
	auto tick = duration_cast<duration<double>>(m_currTime - m_lastTime);
	m_lastTime = m_currTime;
	double clockDen = static_cast<double>(steady_clock::period::den);
	float timeScale = static_cast<float>(1.0 / (tick.count() * clockDen));

	Vector3 pos = m_pCamera->GetTranslation();
	Vector3 fw = m_pCamera->GetForward();
	Vector3 right = m_pCamera->GetRight();

	Vector3 moveCommandV(0.0f);
	if (m_bMoveState[POSITIVE_Z]) moveCommandV += fw;
	if (m_bMoveState[NEGATIVE_Z]) moveCommandV -= fw;
	if (m_bMoveState[POSITIVE_X]) moveCommandV += right;
	if (m_bMoveState[NEGATIVE_X]) moveCommandV -= right;

	float moveSpeed = 30.0f;
	switch (m_bSpeedState)
	{
	case SPEED_LOW: moveSpeed = 15.0f; break;
	case SPEED_MID: moveSpeed = 30.0f; break;
	case SPEED_HIGH: moveSpeed = 50.0f; break;
	}
	printf("%f\n", moveSpeed);

	Vector3 result = pos + moveCommandV * moveSpeed * timeScale;
	m_pCamera->SetTranslation(result);
}

void NSFirstPersonalCamera::OnKeyDown(NXEventArg eArg)
{
	if (eArg.VKey == 'W') m_bMoveState[POSITIVE_Z] = true;
	if (eArg.VKey == 'S') m_bMoveState[NEGATIVE_Z] = true;
	if (eArg.VKey == 'D') m_bMoveState[POSITIVE_X] = true;
	if (eArg.VKey == 'A') m_bMoveState[NEGATIVE_X] = true;

	if (eArg.VKey == NXKeyCode::LeftShift) m_bSpeedState = SPEED_HIGH;
	if (eArg.VKey == NXKeyCode::LeftControl) m_bSpeedState = SPEED_LOW;
}

void NSFirstPersonalCamera::OnKeyUp(NXEventArg eArg)
{
	if (eArg.VKey == 'W') m_bMoveState[POSITIVE_Z] = false;
	if (eArg.VKey == 'S') m_bMoveState[NEGATIVE_Z] = false;
	if (eArg.VKey == 'D') m_bMoveState[POSITIVE_X] = false;
	if (eArg.VKey == 'A') m_bMoveState[NEGATIVE_X] = false;

	if (eArg.VKey == NXKeyCode::LeftShift) m_bSpeedState = SPEED_MID;
	if (eArg.VKey == NXKeyCode::LeftControl) m_bSpeedState = SPEED_MID;
}

void NSFirstPersonalCamera::OnMouseDown(NXEventArg eArg)
{
}

void NSFirstPersonalCamera::OnMouseMove(NXEventArg eArg)
{
	float fYaw = (float)eArg.LastX * m_fSensitivity;
	float fPitch = (float)eArg.LastY * m_fSensitivity;

	Vector3 vUp = m_pCamera->GetUp();
	Vector3 vRight = m_pCamera->GetRight();

	Matrix mxOld = Matrix::CreateFromQuaternion(m_pCamera->GetRotation());
	Matrix mxRot = Matrix::CreateFromAxisAngle(vRight, fPitch) * Matrix::CreateFromAxisAngle(vUp, fYaw);
	Matrix mxNew = mxOld * mxRot;

	Vector3 ignore;
	Quaternion rotation;
	mxNew.Decompose(ignore, rotation, ignore);
	m_pCamera->SetRotation(rotation);
}
