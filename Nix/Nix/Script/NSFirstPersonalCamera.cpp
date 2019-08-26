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

	// 输出Camera当前transform信息
	if (eArg.VKey == 'P')
	{
		string name = m_pCamera->GetName();
		Vector3 t = m_pCamera->GetTranslation();
		Vector3 r = m_pCamera->GetRotation();
		printf("Camera info: name: %s translation: %.3f, %.3f, %.3f rotation: %.3f, %.3f, %.3f\n", 
			name.c_str(), t.x, t.y, t.z, r.x, r.y, r.z);
	}
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

	Vector3 test = m_pCamera->GetRotation();
	printf("%f, %f\t%f\t%f\t%f\t", fYaw, fPitch, test.x, test.y, test.z);

	Matrix mxOld = Matrix::CreateFromXYZ(m_pCamera->GetRotation());
	Matrix mxRotUp = Matrix::CreateFromAxisAngle(vUp, fYaw);
	Matrix mxRotRight = Matrix::CreateFromAxisAngle(vRight, fPitch);

	Vector3 result = (mxRotUp * mxRotRight * mxOld).EulerXYZ();
	m_pCamera->SetRotation(result);

	Vector3 g(0.0f, 1.0f, 0.0f);
	Vector3 g2 = g;
	Matrix m = Matrix::CreateFromXYZ(Vector3(XM_PIDIV2, 0.0f, 0.0f));
	Matrix m2 = Matrix::CreateFromYawPitchRoll(0.0f, XM_PIDIV2, 0.0f);
	g = g.Transform(g, m);
	g2 = g2.Transform(g2, m2);

	test = { 0.0f, 0.0f, 1.0f };
	Vector3 yana = Matrix::CreateFromXYZ(test).EulerXYZ();
	printf("%f\t%f\t%f\n", test.x, test.y, test.z);
}
