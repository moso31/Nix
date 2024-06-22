#include "NXCamera.h"
#include "BaseDefs/NixCore.h"
#include "NXGlobalDefinitions.h"
#include "DirectResources.h"
#include "NSFirstPersonalCamera.h"
#include "NXAllocatorManager.h"

NXCamera::NXCamera(const std::string& name) :
	NXTransform(),
	m_at(0.0f, 0.0f, 0.0f),
	m_up(0.0f, 1.0f, 0.0f),
	m_near(0.1f),
	m_far(1000.0f),
	m_aspectRatio(1.0f),
	m_rtSize(0.0f, 0.0f)
{
}

void NXCamera::SetTranslation(const Vector3& value)
{
	m_translation = value;
	Vector3 dir = Vector3::TransformNormal(Vector3(0.0f, 0.0f, 1.0f), Matrix::CreateFromZXY(m_eulerAngle));

	m_at = m_translation + dir;
	m_up = { 0.0f, 1.0f, 0.0f };
}

void NXCamera::SetRotation(const Vector3& value)
{
	m_eulerAngle = value;
	Vector3 dir = Vector3::TransformNormal(Vector3(0.0f, 0.0f, 1.0f), Matrix::CreateFromZXY(m_eulerAngle));

	m_at = m_translation + dir;
	m_up = { 0.0f, 1.0f, 0.0f };
}

void NXCamera::SetLookAt(Vector3 value)
{
	m_at = value;
	m_up = { 0.0f, 1.0f, 0.0f };

	Vector3 vForward = (m_at - m_translation);
	vForward.Normalize();
	Vector3 vAxis = Vector3(0.0f, 0.0f, 1.0f).Cross(vForward);
	if (vAxis.LengthSquared() == 0.0f) vAxis.y = 1.0f;
	vAxis.Normalize();

	float fAngle = Vector3::AngleNormalize(vForward, Vector3(0.0f, 0.0f, 1.0f));
	Matrix mxAxisAngle = Matrix::CreateFromAxisAngle(vAxis, fAngle);

	m_eulerAngle = mxAxisAngle.EulerRollPitchYaw();
}

Vector3 NXCamera::GetForward()
{
	Vector3 result = (m_at - m_translation);
	result.Normalize();
	if (result.IsZero())
		result = Vector3(0.0f, 0.0f, 1.0f);
	return result;
}

Vector3 NXCamera::GetLeft()
{
	Vector3 result = (m_at - m_translation).Cross(m_up);
	result.Normalize();
	if (result.IsZero())
		result = Vector3(-1.0f, 0.0f, 0.0f);
	return result;
}

Vector3 NXCamera::GetRight()
{
	Vector3 result = (m_translation - m_at).Cross(m_up);
	result.Normalize();
	if (result.IsZero())
		result = Vector3(1.0f, 0.0f, 0.0f);
	return result;
}

Vector3 NXCamera::GetAt()
{
	return m_at;
}

Vector3 NXCamera::GetUp()
{
	return m_up;
}

const Matrix& NXCamera::GetViewMatrix()
{
	return m_mxView;
}

const Matrix& NXCamera::GetViewInverseMatrix()
{
	return m_mxViewInv;
}

const Matrix& NXCamera::GetProjectionMatrix()
{
	return m_mxProjection;
}

const Matrix& NXCamera::GetProjectionInverseMatrix()
{
	return m_mxProjectionInv;
}

const Matrix& NXCamera::GetViewProjectionMatrix()
{
	return m_mxViewProjection;
}

const Matrix& NXCamera::GetViewProjectionInverseMatrix()
{
	return m_mxViewProjectionInv;
}

Ray NXCamera::GenerateRay(const Vector2& cursor, const Vector2& rectSize)
{
	float x = (2.0f * cursor.x / rectSize.x - 1.0f) / m_mxProjection._11;
	float y = (1.0f - 2.0f * cursor.y / rectSize.y) / m_mxProjection._22;

	Vector3 vOrig(0.0f);
	Vector3 vDir = Vector3(x, y, 1.0f);
	vDir.Normalize();

	Vector3 vOrigWorld = Vector3::Transform(vOrig, m_mxViewInv);
	Vector3 vDirWorld = Vector3::TransformNormal(vDir, m_mxViewInv);

	return Ray(vOrigWorld, vDirWorld);
}

void NXCamera::Init(float fovY, float zNear, float zFar, Vector3 cameraPosition, Vector3 cameraLookAt, Vector3 cameraLookUp, const Vector2& rtSize)
{
	m_fovY = fovY;
	m_near = zNear;
	m_far = zFar;

	SetTranslation(cameraPosition);
	SetLookAt(cameraLookAt);
	m_up = cameraLookUp;

	NXGlobalBuffer::cbCamera.CreateFrameBuffers(NXCBufferAllocator, NXDescriptorAllocator);

	OnResize(rtSize);
}

void NXCamera::OnResize(const Vector2& rtSize)
{
	m_rtSize = rtSize;
	m_aspectRatio = m_rtSize.x / m_rtSize.y;
}

void NXCamera::UpdateTransform()
{
	m_mxView = XMMatrixLookAtLH(m_translation, m_at, m_up);
	m_mxViewInv = m_mxView.Invert();

	m_fovY = Clamp(m_fovY, 0.001f, 179.999f);
	m_mxProjection = XMMatrixPerspectiveFovLH(m_fovY * XM_PI / 180.0f, m_aspectRatio, m_near, m_far);
	m_mxProjectionInv = m_mxProjection.Invert();

	m_mxViewProjection = m_mxView * m_mxProjection;
	m_mxViewProjectionInv = m_mxViewProjection.Invert();

	NXTransform::UpdateTransform();
}

void NXCamera::Update()
{
	auto& cbDataObject = NXGlobalBuffer::cbObject.Get();
	cbDataObject.view = m_mxView.Transpose();
	cbDataObject.viewInverse = m_mxViewInv.Transpose();
	cbDataObject.viewInverseTranspose = m_mxViewInv;
	cbDataObject.viewTranspose = m_mxView;
	cbDataObject.projection = m_mxProjection.Transpose();
	cbDataObject.projectionInverse = m_mxProjectionInv.Transpose();
	NXGlobalBuffer::cbObject.UpdateBuffer();

	auto& cbDataCamera = NXGlobalBuffer::cbCamera.Get();
	float invN2F = 1.0f / (m_far - m_near);
	cbDataCamera.Params0 = Vector4(m_rtSize.x, m_rtSize.y, 1.0f / m_rtSize.x, 1.0f / m_rtSize.y);
	cbDataCamera.Params1 = Vector4(m_near, m_far, m_far * invN2F, -m_far * m_near * invN2F);
	cbDataCamera.Params2 = Vector4(m_mxProjection._11, m_mxProjection._22, 1.0f / m_mxProjection._11, 1.0f / m_mxProjection._22);
	NXGlobalBuffer::cbCamera.UpdateBuffer();
}

void NXCamera::Render()
{
}

void NXCamera::Release()
{
	NXObject::Release();
}

NSFirstPersonalCamera* NXCamera::GetFirstPersonalController()
{
	return m_pEditorFPCScript;
}

void NXCamera::SetFirstPersonalController(NSFirstPersonalCamera* pScript)
{
	m_pEditorFPCScript = pScript;
}
