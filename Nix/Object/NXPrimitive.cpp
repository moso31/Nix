#include "NXPrimitive.h"

NXPrimitive::NXPrimitive()
{
}

NXPrimitive::~NXPrimitive()
{
}

void NXPrimitive::SetMaterial(const shared_ptr<NXMaterial>& pMaterial)
{
	m_pMaterial = pMaterial;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferMaterial);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HRESULT hr = g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbMaterial);
	if (FAILED(hr))
		return;

	m_cbDataMaterial.material = pMaterial->GetMaterialInfo();
}

