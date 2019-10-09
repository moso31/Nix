#include "NXCamera.h"
#include "DirectResources.h"
#include "GlobalBufferManager.h"

NXCamera::NXCamera() :
	NXTransform(),
	m_at(0.0f, 0.0f, 0.0f),
	m_up(0.0f, 1.0f, 0.0f)
{
	m_name = "Camera";
}

void NXCamera::SetTranslation(Vector3 value)
{
	m_translation = value;
	Vector3 dir = Vector3::TransformNormal(Vector3(0.0f, 0.0f, 1.0f), Matrix::CreateFromQuaternion(m_rotation));

	m_at = m_translation + dir;
	m_up = { 0.0f, 1.0f, 0.0f };

	m_view.CreateLookAt(m_translation, m_at, m_up);
}

void NXCamera::SetRotation(Quaternion value)
{
	m_rotation = value;
	Vector3 dir = Vector3::TransformNormal(Vector3(0.0f, 0.0f, 1.0f), Matrix::CreateFromQuaternion(m_rotation));

	m_at = m_translation + dir;
	m_up = { 0.0f, 1.0f, 0.0f };

	m_view.CreateLookAt(m_translation, m_at, m_up);
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
	m_rotation = Quaternion::CreateFromAxisAngle(vAxis, fAngle);

	m_view.CreateLookAt(m_translation, m_at, m_up);
}

Vector3 NXCamera::GetForward()
{
	Vector3 result = (m_at - m_translation);
	result.Normalize();
	return result;
}

Vector3 NXCamera::GetLeft()
{
	Vector3 result = (m_at - m_translation).Cross(m_up);
	result.Normalize();
	return result;
}

Vector3 NXCamera::GetRight()
{
	Vector3 result = (m_translation - m_at).Cross(m_up);
	result.Normalize();
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

Ray NXCamera::GenerateRay(Vector2 cursorPosition)
{
	Vector2 outputSize = g_dxResources->GetViewSize();

	float x = (2.0f * cursorPosition.x / outputSize.x - 1.0f) / m_projection._11;
	float y = (1.0f - 2.0f * cursorPosition.y / outputSize.y) / m_projection._22;

	Vector3 vOrig(0.0f);
	Vector3 vDir = Vector3(x, y, 1.0f);
	vDir.Normalize();

	Matrix viewInv = m_view.Invert();
	Vector3 vOrigWorld = Vector3::Transform(vOrig, viewInv);
	Vector3 vDirWorld = Vector3::TransformNormal(vDir, viewInv);

	return Ray(vOrigWorld, vDirWorld);
}

void NXCamera::Init(Vector3 cameraPosition, Vector3 cameraLookAt, Vector3 cameraLookUp)
{
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
}

void NXCamera::PrevUpdate()
{
	Vector2 vpsz = g_dxResources->GetViewPortSize();
	float aspectRatio = vpsz.x / vpsz.y;
	m_view = XMMatrixLookAtLH(m_translation, m_at, m_up);
	m_projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, aspectRatio, 0.01f, 1000.0f);

	NXTransform::PrevUpdate();
}

void NXCamera::Update()
{
	NXGlobalBufferManager::m_cbDataCamera.view = m_view.Transpose();
	NXGlobalBufferManager::m_cbDataCamera.projection = m_projection.Transpose();
	NXGlobalBufferManager::m_cbDataCamera.eyePosition = m_translation;

	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbCamera, 0, nullptr, &NXGlobalBufferManager::m_cbDataCamera, 0, 0);
}

void NXCamera::Render()
{
}

void NXCamera::Release()
{
	NXObject::Release();
}
