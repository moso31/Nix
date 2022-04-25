#include "NXCamera.h"
#include "DirectResources.h"
#include "GlobalBufferManager.h"

NXCamera::NXCamera() :
	NXTransform(),
	m_at(0.0f, 0.0f, 0.0f),
	m_up(0.0f, 1.0f, 0.0f),
	m_near(0.1f),
	m_far(1000.0f)
{
	m_name = "Camera";
	m_type = NXType::eCamera;
}

void NXCamera::SetTranslation(const Vector3& value)
{
	m_translation = value;
	Vector3 dir = Vector3::TransformNormal(Vector3(0.0f, 0.0f, 1.0f), Matrix::CreateFromRollPitchYaw(m_eulerAngle));

	m_at = m_translation + dir;
	m_up = { 0.0f, 1.0f, 0.0f };
}

void NXCamera::SetRotation(const Vector3& value)
{
	m_eulerAngle = value;
	Vector3 dir = Vector3::TransformNormal(Vector3(0.0f, 0.0f, 1.0f), Matrix::CreateFromRollPitchYaw(m_eulerAngle));

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
	return m_view;
}

const Matrix& NXCamera::GetProjectionMatrix()
{
	return m_projection;
}

Ray NXCamera::GenerateRay(const Vector2& cursorPosition)
{
	return GenerateRay(cursorPosition, g_dxResources->GetViewSize());
}

Ray NXCamera::GenerateRay(const Vector2& cursor, const Vector2& imageSize)
{
	float x = (2.0f * cursor.x / imageSize.x - 1.0f) / m_projection._11;
	float y = (1.0f - 2.0f * cursor.y / imageSize.y) / m_projection._22;

	Vector3 vOrig(0.0f);
	Vector3 vDir = Vector3(x, y, 1.0f);
	vDir.Normalize();

	Matrix viewInv = m_view.Invert();
	Vector3 vOrigWorld = Vector3::Transform(vOrig, viewInv);
	Vector3 vDirWorld = Vector3::TransformNormal(vDir, viewInv);

	return Ray(vOrigWorld, vDirWorld);
}

void NXCamera::Init(float fovY, float zNear, float zFar, Vector3 cameraPosition, Vector3 cameraLookAt, Vector3 cameraLookUp)
{
	m_fovY = fovY;
	m_near = zNear;
	m_far = zFar;

	SetTranslation(cameraPosition);
	SetLookAt(cameraLookAt);
	m_up = cameraLookUp;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferCamera);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &NXGlobalBufferManager::m_cbCamera));

	float nInv = 1.0f / m_near;
	NXGlobalBufferManager::m_cbDataCamera.Params1 = { 1.0f - m_far * nInv, m_far * nInv, 1.0f / m_far - nInv, nInv }; 
}

void NXCamera::UpdateTransform()
{
	Vector2 vpsz = g_dxResources->GetViewPortSize();
	float aspectRatio = vpsz.x / vpsz.y;
	m_view = XMMatrixLookAtLH(m_translation, m_at, m_up);
	m_projection = XMMatrixPerspectiveFovLH(m_fovY * XM_PI / 180.0f, aspectRatio, m_near, m_far);

	NXTransform::UpdateTransform();
}

void NXCamera::Update()
{
	NXGlobalBufferManager::m_cbDataObject.view = m_view.Transpose();
	NXGlobalBufferManager::m_cbDataObject.viewInverse = m_view.Invert().Transpose(); 
	NXGlobalBufferManager::m_cbDataObject.viewTranspose = m_view;
	NXGlobalBufferManager::m_cbDataObject.projection = m_projection.Transpose();
	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbObject.Get(), 0, nullptr, &NXGlobalBufferManager::m_cbDataObject, 0, 0);

	// 【这两行临时的。buffersize不该和Camera放在一起。回头改】
	Vector2 viewSize = g_dxResources->GetViewSize();
	NXGlobalBufferManager::m_cbDataCamera.Params0 = Vector4(viewSize.x, viewSize.y, 1.0f / viewSize.x, 1.0f / viewSize.y);

	NXGlobalBufferManager::m_cbDataCamera.Params2 = Vector4(m_projection._11, m_projection._22, 1.0f / m_projection._11, 1.0f / m_projection._22);
	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbCamera.Get(), 0, nullptr, &NXGlobalBufferManager::m_cbDataCamera, 0, 0);
}

void NXCamera::Render()
{
}

void NXCamera::Release()
{
	NXObject::Release();
}
