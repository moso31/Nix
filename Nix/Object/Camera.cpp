#include "Camera.h"
#include "DirectResources.h"

HRESULT Camera::Init()
{
	// Initialize the view matrix
	Vector4 Eye(0.0f, 0.0f, -1.5f, 0.0f);
	Vector4 At(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 Up(0.0f, 1.0f, 0.0f, 0.0f);
	m_pConstantBufferData.view = XMMatrixLookAtLH(Eye, At, Up);

	// Initialize the projection matrix
	Vector2 vpsz = g_dxResources->GetViewPortSize();
	m_pConstantBufferData.projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, vpsz.x / vpsz.y, 0.01f, 100.0f);

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferCamera);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	HRESULT hr;
	hr = g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_pConstantBuffer);
	if (FAILED(hr))
		return hr;

	ConstantBufferCamera cb;
	cb.view = m_pConstantBufferData.view.Transpose();
	cb.projection = m_pConstantBufferData.projection.Transpose();
	cb.eyePosition = Vector3(Eye);

	g_pContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	return S_OK;
}

void Camera::Update()
{
}

void Camera::Render()
{
	g_pContext->VSSetConstantBuffers(1, 1, &m_pConstantBuffer);
	g_pContext->PSSetConstantBuffers(1, 1, &m_pConstantBuffer);
}

void Camera::Release()
{
}
