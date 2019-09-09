#include "NXMesh.h"
#include "FBXMeshLoader.h"

NXMesh::NXMesh()
{
}

NXMesh::~NXMesh()
{
}

void NXMesh::Init(string filePath)
{
	FBXMeshLoader::LoadFBXFile(filePath, this);

	for (int i = 0; i < m_vertices.size(); i++)
	{
		//m_aabb.Merge(m_vertices[i].pos);
	}

	m_filePath = filePath;

	InitVertexIndexBuffer();
	InitAABB();
}

void NXMesh::Render()
{
	UINT stride = sizeof(VertexPNT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	g_pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	g_pContext->PSSetShaderResources(0, 1, &m_pTextureSRV);
	g_pContext->PSSetConstantBuffers(3, 1, &m_cbMaterial);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

void NXMesh::Release()
{
	if (m_pVertexBuffer)	m_pVertexBuffer->Release();
	if (m_pIndexBuffer)		m_pIndexBuffer->Release();
	if (m_pConstantBuffer)	m_pConstantBuffer->Release();
	if (m_pTextureSRV)		m_pTextureSRV->Release();
}
