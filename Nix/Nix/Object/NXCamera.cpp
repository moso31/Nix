#include "NXCamera.h"
#include "DirectResources.h"

NXCamera::NXCamera() :
	NXTransform(),
	m_at(0.0f, 0.0f, 0.0f),
	m_up(0.0f, 1.0f, 0.0f)
{
	m_name = "Camera";
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
