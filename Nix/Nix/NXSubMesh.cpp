#include "NXSubMesh.h"

NXTriangle::NXTriangle(NXSubMesh* pSubMesh, int startIndex) :
	pSubMesh(pSubMesh),
	startIndex(startIndex)
{
}

float NXTriangle::Area() const
{
	Vector3 P0 = GetVertex(0).pos;
	Vector3 P1 = GetVertex(1).pos;
	Vector3 P2 = GetVertex(2).pos;
	return 0.5f * (P1 - P0).Cross(P2 - P0).Length();
}

VertexPNTT NXTriangle::GetVertex(int VertexId) const
{
	assert(VertexId >= 0 && VertexId < 3);
	return pSubMesh->m_vertices[pSubMesh->m_indices[startIndex + VertexId]];
}

bool NXTriangle::RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist)
{
	VertexPNTT data0 = pSubMesh->m_vertices[pSubMesh->m_indices[startIndex]];
	VertexPNTT data1 = pSubMesh->m_vertices[pSubMesh->m_indices[startIndex + 1]];
	VertexPNTT data2 = pSubMesh->m_vertices[pSubMesh->m_indices[startIndex + 2]];

	Vector3 p0 = data0.pos;
	Vector3 p1 = data1.pos;
	Vector3 p2 = data2.pos;

	// ������ռ�ת�������߿ռ䣺World * T * P * S = Ray��

	// T�任��ֻ�Ǽ򵥵�ƽ�ơ�
	Vector3 p0t = p0 - localRay.position;
	Vector3 p1t = p1 - localRay.position;
	Vector3 p2t = p2 - localRay.position;

	// P�任��xyz���û���z��Զ��Ϊ����ᡣ֮��xy������䡣
	// ���������һ��������Ϊ����ᣬ�Ծ����ܼ��ٷ���任�ĸ��㾫����ʧ��
	int kz = Vector3::Abs(localRay.direction).MaxDimension();
	int kx = kz + 1;
	if (kx == 3) kx = 0;
	int ky = kx + 1;
	if (ky == 3) ky = 0;

	Vector3 d = Vector3::Permute(localRay.direction, kx, ky, kz);
	p0t = Vector3::Permute(p0t, kx, ky, kz);
	p1t = Vector3::Permute(p1t, kx, ky, kz);
	p2t = Vector3::Permute(p2t, kx, ky, kz);

	// S�任������任��
	float shearX = -d.x / d.z;
	float shearY = -d.y / d.z;
	float shearZ = 1.0f / d.z;

	p0t.x += shearX * p0t.z;
	p1t.x += shearX * p1t.z;
	p2t.x += shearX * p2t.z;

	p0t.y += shearY * p0t.z;
	p1t.y += shearY * p1t.z;
	p2t.y += shearY * p2t.z;

	// ����任�������ξ��Ѿ���ת�������߿ռ��ˡ�
	// ֮�󼴿��������߿ռ��еı�Ե����e0 e1 e2��
	float e0 = p1t.x * p2t.y - p1t.y * p2t.x;
	float e1 = p2t.x * p0t.y - p2t.y * p0t.x;
	float e2 = p0t.x * p1t.y - p0t.y * p1t.x;

	// ���߲��ԣ�e0, e1, e2ͬΪ������������㸺����˵��p���������ڡ�
	if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0))
		return false;
	// �洢������Ե����ֵ���ܺͣ��Ժ�����������á�
	float det = e0 + e1 + e2;
	// ���������Ե����ֵ���ܺ�Ϊ�㣬˵�������������������α��ϡ������������δ���С�
	// ֻҪ����Mesh����ȫ��յģ������������������������о�һ���ܵõ�һ��������det != 0�ķ�����
	if (det == 0) return false;

	// ��ɻ�δ����Z��S����任��
	p0t.z *= shearZ;
	p1t.z *= shearZ;
	p2t.z *= shearZ;

	// ����е��tֵ
	float tScaled = e0 * p0t.z + e1 * p1t.z + e2 * p2t.z;

	// ���ֻ��������α���������det>0��t<=0����det<0��t>=0����
	// �������������Ϊ�����С�
	// ����ʹ��tScaled�ж϶�����t����Ϊ�����t��Ҫִ��һ�γ�������������������
	// �������ж��ǲ���Ҫ���г����ġ�
	if (det < 0 && (tScaled >= 0))
		return false;
	else if (det > 0 && (tScaled <= 0))
		return false;

	// ������������b0 b1 b2 + ���е��tֵ
	float invDet = 1 / det;
	float b0 = e0 * invDet;
	float b1 = e1 * invDet;
	float b2 = e2 * invDet;
	float t = tScaled * invDet;

	// �����Ѿ�ȷ�����У���������t�����ߵ������εľ��룩��

	// �ж�t�Ƿ��ǵ�ǰ������㡣�ǵĻ��ͼ���dpdu��dpdv�����ݣ�������outHitInfo��
	if (outDist > t)
	{
		// ����dpdu��dpdv��
		Vector3 dpdu, dpdv;
		Vector2 uv[3];
		uv[0] = data0.tex;
		uv[1] = data1.tex;
		uv[2] = data2.tex;

		float u0 = uv[0][0];
		float u1 = uv[1][0];
		float u2 = uv[2][0];
		float v0 = uv[0][1];
		float v1 = uv[1][1];
		float v2 = uv[2][1];

		float du02 = u0 - u2;
		float du12 = u1 - u2;
		float dv02 = v0 - v2;
		float dv12 = v1 - v2;
		float uvdet = du02 * dv12 - dv02 * du12;
		float uvdetInv = 0.0f;
		Vector3 dp02 = p0 - p2;
		Vector3 dp12 = p1 - p2;
		if (abs(uvdet) >= 1e-6)
		{
			uvdetInv = 1.0f / uvdet;
			dpdu = (dp02 * dv12 - dp12 * dv02) * uvdetInv;
			dpdv = (dp12 * du02 - dp02 * du12) * uvdetInv;
		}
		dpdu.Normalize();
		dpdv.Normalize();

		// ���uv����ʽ���=0����ô�޷��������Чdpdu dpdv��
		// �����������Ҫǿ��Ϊ����������һ������ϵ��
		if (abs(uvdet) < 1e-6 || dpdu.Cross(dpdv).LengthSquared() == 0.0f)
		{
			Vector3 ng = dp12.Cross(dp02);
			if (ng.LengthSquared() == 0.0f)
				return false;

			ng.Normalize();
			ng.GenerateCoordinateSpace(dpdu, dpdv);
		}

		// �������λ�õ�p��UV
		Vector3 pHit = b0 * p0 + b1 * p1 + b2 * p2;
		Vector2 uvHit = b0 * uv[0] + b1 * uv[1] + b2 * uv[2];

		outHitInfo = NXHit(pSubMesh, pHit, uvHit, -localRay.direction, dpdu, dpdv);
		outDist = t;
	}

	return true;
}

NXSubMesh::NXSubMesh(NXPrimitive* pPrimitive) :
	m_pPBRMaterial(nullptr),
	m_parent(pPrimitive)
{
}

NXSubMesh::~NXSubMesh()
{
}

void NXSubMesh::Update()
{
	if (m_pPBRMaterial)
	{
		// 2021.3.3 lazyģʽ��ֱ����һ����Ҫ����Material��ʱ��ż���MaterialBuffer��
		// Ŀǰ��������ʱû�����⡣���������Ҫ֧�ֶ���ʣ�������ܾ���Ҫ�Ķ��ˡ�
		if (!m_cbMaterial) InitMaterialBuffer();
		g_pContext->UpdateSubresource(m_cbMaterial.Get(), 0, nullptr, &m_cbDataMaterial, 0, 0);
	}
}

void NXSubMesh::Render()
{
	UINT stride = sizeof(VertexPNTT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

NXPBRMaterial* NXSubMesh::GetPBRMaterial() const
{
	return m_pPBRMaterial;
}

void NXSubMesh::SetMaterialPBR(NXPBRMaterial* mat)
{
	m_pPBRMaterial = mat;
	m_cbDataMaterial = m_pPBRMaterial->GetConstantBuffer();
}

void NXSubMesh::CalculateTangents(bool bUpdateVertexIndexBuffer)
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

	if (bUpdateVertexIndexBuffer)
	{
		InitVertexIndexBuffer();
	}
}

NXTriangle NXSubMesh::GetFaceTriangle(UINT faceIndex)
{
	return NXTriangle(this, faceIndex * 3);
}

void NXSubMesh::InitVertexIndexBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexPNTT) * (UINT)m_vertices.size();
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

void NXSubMesh::InitMaterialBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferMaterial);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbMaterial));
}