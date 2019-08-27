#include "NXCamera.h"
#include "DirectResources.h"

NXCamera::NXCamera() :
	NXTransform(),
	m_at(0.0f, 0.0f, 0.0f),
	m_up(0.0f, 1.0f, 0.0f),
	m_cbCamera(nullptr)
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

	float fAngle = Vector3::AngleNormalize(vForward, Vector3(0.0f, 0.0f, 1.0f));
	m_rotation = Quaternion(vAxis, fAngle);

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

void NXCamera::Init(Vector3 cameraPosition, Vector3 cameraLookAt, Vector3 cameraLookUp)
{
	SetTranslation(cameraPosition);
	m_at = cameraLookAt;
	m_up = cameraLookUp;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferCamera);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbCamera));
}

void NXCamera::PrevUpdate()
{
	Vector2 vpsz = g_dxResources->GetViewPortSize();
	float aspectRatio = vpsz.x / vpsz.y;
	m_view = XMMatrixLookAtLH(m_translation, m_at, m_up);
	m_projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, aspectRatio, 0.01f, 100.0f);

	NXTransform::PrevUpdate();
}

void NXCamera::Update()
{
	m_cbDataCamera.view = m_view.Transpose();
	m_cbDataCamera.projection = m_projection.Transpose();
	m_cbDataCamera.eyePosition = m_translation;

	g_pContext->UpdateSubresource(m_cbCamera, 0, nullptr, &m_cbDataCamera, 0, 0);
}

void NXCamera::Render()
{
	g_pContext->VSSetConstantBuffers(1, 1, &m_cbCamera);
	g_pContext->PSSetConstantBuffers(1, 1, &m_cbCamera);
}

void NXCamera::Release()
{
}
