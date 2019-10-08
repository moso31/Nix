#include "NXPrimitive.h"
#include "NXMaterial.h"
#include "WICTextureLoader.h"
#include "GlobalBufferManager.h"

NXPrimitive::NXPrimitive() :
	m_pVertexBuffer(nullptr),
	m_pIndexBuffer(nullptr),
	m_pTextureSRV(nullptr),
	m_cbMaterial(nullptr)
{
}

void NXPrimitive::Update()
{
	NXGlobalBufferManager::m_cbDataWorld.world = m_worldMatrix.Transpose();
	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbWorld, 0, nullptr, &NXGlobalBufferManager::m_cbDataWorld, 0, 0);

	if (m_pMaterial)
	{
		g_pContext->UpdateSubresource(m_cbMaterial, 0, nullptr, &m_cbDataMaterial, 0, 0);
	}
}

void NXPrimitive::Render()
{
	UINT stride = sizeof(VertexPNT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

void NXPrimitive::Release()
{
	if (m_pVertexBuffer)	m_pVertexBuffer->Release();
	if (m_pIndexBuffer)		m_pIndexBuffer->Release();
	if (m_pTextureSRV)		m_pTextureSRV->Release();

	m_pMaterial.reset();
	if (m_cbMaterial)		m_cbMaterial->Release();
	NXObject::Release();
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
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbMaterial));

	m_cbDataMaterial = pMaterial->GetMaterialInfo();
}

AABB NXPrimitive::GetAABB() const
{
	return m_aabb;
}

bool NXPrimitive::Intersect(const Ray& Ray, Vector3& outHitPos, float& outDist)
{
	int outIndex = -1;
	outDist = FLT_MAX;
	for (int i = 0; i < (int)m_indices.size() / 3; i++)
	{
		Vector3 P0 = m_vertices[m_indices[i * 3 + 0]].pos;
		Vector3 P1 = m_vertices[m_indices[i * 3 + 1]].pos;
		Vector3 P2 = m_vertices[m_indices[i * 3 + 2]].pos;

		float dist;
		if (Ray.Intersects(P0, P1, P2, dist))
		{
			if (dist < outDist)
			{
				outDist = dist;
				outIndex = i;
			}
		}
	}

	if (outIndex != -1)
	{
		outHitPos = Ray.position + Ray.direction * outDist;
		return true;
	}
	else
		return false;
}

void NXPrimitive::InitVertexIndexBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexPNT) * (UINT)m_vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = m_vertices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(USHORT) * (UINT)m_indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = m_indices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBuffer));
	
	// TODO：纹理的SRV应该改成全局通用的
	NX::ThrowIfFailed(CreateWICTextureFromFile(g_pDevice, L"D:\\rgb.bmp", nullptr, &m_pTextureSRV));
}

void NXPrimitive::InitAABB()
{
	for (auto it = m_vertices.begin(); it != m_vertices.end(); it++)
	{
		m_points.push_back(it->pos);
	}
	AABB::CreateFromPoints(m_aabb, m_points.size(), m_points.data(), sizeof(Vector3));
}
