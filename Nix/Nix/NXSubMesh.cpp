#include "NXSubMesh.h"
#include "NXPrimitive.h"

template class NXSubMesh<VertexPNTT>;
template class NXSubMesh<VertexEditorObjects>;

void NXSubMeshBase::UpdateViewParams() 
{
	m_pPrimitive->UpdateViewParams(); 
}

template<class TVertex>
void NXSubMesh<TVertex>::Render()
{
	UINT stride = sizeof(TVertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

template<class TVertex>
bool NXSubMesh<TVertex>::RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist)
{
	bool bSuccess = false;
	for (UINT i = 0, faceId = 0; i < m_indices.size(); i += 3, faceId++)
	{
		Triangle face(m_vertices[m_indices[i + 0]].pos, m_vertices[m_indices[i + 1]].pos, m_vertices[m_indices[i + 2]].pos);
		if (face.Intersects(localRay, outHitInfo.position, outDist))
		{
			outHitInfo.pSubMesh = this;
			outHitInfo.faceIndex = faceId;
			bSuccess = true;
		}
	}

	return bSuccess;
}

template<class TVertex>
void NXSubMesh<TVertex>::CalcLocalAABB()
{
	Vector3 vMin(FLT_MAX);
	Vector3 vMax(FLT_MIN);

	for (int i = 0; i < m_vertices.size(); i++)
	{
		vMin = Vector3::Min(vMin, m_vertices[i].pos);
		vMax = Vector3::Max(vMax, m_vertices[i].pos);
	}

	m_localAABB = AABB(vMin, vMax);
}

template<class TVertex>
void NXSubMesh<TVertex>::UpdateVBIB()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(TVertex) * (UINT)m_vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = m_vertices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(UINT) * (UINT)m_indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = m_indices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBuffer));
}

void NXSubMeshStandard::CalculateTangents(bool bUpdateVBIB)
{
	int faceCount = (int)m_indices.size() / 3;
	for (int i = 0; i < m_indices.size(); i += 3)
	{
		auto& P0 = m_vertices[m_indices[i + 0]];
		auto& P1 = m_vertices[m_indices[i + 1]];
		auto& P2 = m_vertices[m_indices[i + 2]];

		auto e0 = P1.pos - P0.pos;
		auto e1 = P2.pos - P0.pos;

		auto uv0 = P1.tex - P0.tex;
		auto uv1 = P2.tex - P0.tex;

		float u0 = uv0.x;
		float v0 = uv0.y;
		float u1 = uv1.x;
		float v1 = uv1.y;

		float detInvUV = 1.0f / (u0 * v1 - v0 * u1);

		Vector3 dpdu = v1 * e0 - v0 * e1;
		dpdu.Normalize();
		P0.tangent = dpdu;
		P1.tangent = dpdu;
		P2.tangent = dpdu;
	}

	if (bUpdateVBIB)
	{
		UpdateVBIB();
	}
}
