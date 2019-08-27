#include "NSFirstPersonalCamera.h"
#include "NXCamera.h"
#include "NXEvent.h"

NSFirstPersonalCamera::NSFirstPersonalCamera() :
	m_fMoveSpeed(0.03f),
	m_fSensitivity(0.005f)
{
	memset(m_bMoveState, false, sizeof(m_bMoveState));
}

void NSFirstPersonalCamera::SetFPSCamera(const shared_ptr<NXCamera>& pCamera)
{
	m_pCamera = pCamera;
}

void NSFirstPersonalCamera::Update()
{
	Vector3 pos = m_pCamera->GetTranslation();
	Vector3 fw = m_pCamera->GetForward();
	Vector3 right = m_pCamera->GetRight();

	Vector3 moveCommandV(0.0f);
	if (m_bMoveState[POSITIVE_Z]) moveCommandV += fw;
	if (m_bMoveState[NEGATIVE_Z]) moveCommandV -= fw;
	if (m_bMoveState[POSITIVE_X]) moveCommandV += right;
	if (m_bMoveState[NEGATIVE_X]) moveCommandV -= right;
	
	Vector3 result = pos + moveCommandV * m_fMoveSpeed;
	m_pCamera->SetTranslation(result);
}

void NSFirstPersonalCamera::OnKeyDown(NXEventArg eArg)
{
	if (eArg.VKey == 'W') m_bMoveState[POSITIVE_Z] = true;
	if (eArg.VKey == 'S') m_bMoveState[NEGATIVE_Z] = true;
	if (eArg.VKey == 'D') m_bMoveState[POSITIVE_X] = true;
	if (eArg.VKey == 'A') m_bMoveState[NEGATIVE_X] = true;
}

void NSFirstPersonalCamera::OnKeyUp(NXEventArg eArg)
{
	if (eArg.VKey == 'W') m_bMoveState[POSITIVE_Z] = false;
	if (eArg.VKey == 'S') m_bMoveState[NEGATIVE_Z] = false;
	if (eArg.VKey == 'D') m_bMoveState[POSITIVE_X] = false;
	if (eArg.VKey == 'A') m_bMoveState[NEGATIVE_X] = false;
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
	Matrix mxRotUp = Matrix::CreateFromAxisAngle(vUp, fYaw);
	Matrix mxRotRight = Matrix::CreateFromAxisAngle(vRight, fPitch);
	Matrix mxNew = mxRotUp * mxRotRight * mxOld;

	Vector3 ignore;
	Quaternion rotation;
	mxNew.Decompose(ignore, rotation, ignore);
	m_pCamera->SetRotation(rotation);
}
