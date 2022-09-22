#include "NXSubMesh.h"
#include "NXPrimitive.h"

void NXSubMeshBase::UpdateViewParams()
{
	return m_pParent->UpdateViewParams();
}

void NXSubMeshBase::Update()
{
	if (m_pMaterial)
	{
		m_pMaterial->Update();
	}
}

void NXSubMeshBase::Render()
{
	UINT stride = sizeof(VertexPNTT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

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

	// 从世界空间转换到射线空间：World * T * P * S = Ray。

	// T变换：只是简单的平移。
	Vector3 p0t = p0 - localRay.position;
	Vector3 p1t = p1 - localRay.position;
	Vector3 p2t = p2 - localRay.position;

	// P变换：xyz轴置换。z永远作为最大轴。之后xy随意分配。
	// 将射线最长的一个方向作为最大轴，以尽可能减少仿射变换的浮点精度损失。
	int kz = Vector3::Abs(localRay.direction).MaxDimension();
	int kx = kz + 1;
	if (kx == 3) kx = 0;
	int ky = kx + 1;
	if (ky == 3) ky = 0;

	Vector3 d = Vector3::Permute(localRay.direction, kx, ky, kz);
	p0t = Vector3::Permute(p0t, kx, ky, kz);
	p1t = Vector3::Permute(p1t, kx, ky, kz);
	p2t = Vector3::Permute(p2t, kx, ky, kz);

	// S变换：仿射变换。
	float shearX = -d.x / d.z;
	float shearY = -d.y / d.z;
	float shearZ = 1.0f / d.z;

	p0t.x += shearX * p0t.z;
	p1t.x += shearX * p1t.z;
	p2t.x += shearX * p2t.z;

	p0t.y += shearY * p0t.z;
	p1t.y += shearY * p1t.z;
	p2t.y += shearY * p2t.z;

	// 仿射变换后三角形就已经被转换到射线空间了。
	// 之后即可求在射线空间中的边缘函数e0 e1 e2。
	float e0 = p1t.x * p2t.y - p1t.y * p2t.x;
	float e1 = p2t.x * p0t.y - p2t.y * p0t.x;
	float e2 = p0t.x * p1t.y - p0t.y * p1t.x;

	// 射线测试：e0, e1, e2同为非零正数或非零负数，说明p在三角形内。
	if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0))
		return false;
	// 存储三个边缘函数值的总和，对后续计算很有用。
	float det = e0 + e1 + e2;
	// 如果三个边缘函数值的总和为零，说明射线正好落于三角形边上。这种情况视作未击中。
	// 只要三角Mesh是完全封闭的，该射线在其他相邻三角形中就一定能得到一个击中且det != 0的反馈。
	if (det == 0) return false;

	// 完成还未做的Z轴S仿射变换。
	p0t.z *= shearZ;
	p1t.z *= shearZ;
	p2t.z *= shearZ;

	// 求击中点的t值
	float tScaled = e0 * p0t.z + e1 * p1t.z + e2 * p2t.z;

	// 两种击中三角形背面的情况（det>0但t<=0，或det<0但t>=0）。
	// 这两种情况视作为被击中。
	// 另外使用tScaled判断而不是t。因为计算出t需要执行一次除法操作（见后续）。
	// 但符号判断是不需要进行除法的。
	if (det < 0 && (tScaled >= 0))
		return false;
	else if (det > 0 && (tScaled <= 0))
		return false;

	// 计算重心坐标b0 b1 b2 + 击中点的t值
	float invDet = 1 / det;
	float b0 = e0 * invDet;
	float b1 = e1 * invDet;
	float b2 = e2 * invDet;
	float t = tScaled * invDet;

	// 至此已经确定击中，并计算了t（射线到三角形的距离）。

	// 判断t是否是当前最近交点。是的话就计算dpdu，dpdv等数据，并更新outHitInfo。
	if (outDist > t)
	{
		// 计算dpdu和dpdv。
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

		// 如果uv行列式结果=0，那么无法计算出有效dpdu dpdv。
		// 这种情况下需要强行为法向量生成一个坐标系。
		if (abs(uvdet) < 1e-6 || dpdu.Cross(dpdv).LengthSquared() == 0.0f)
		{
			Vector3 ng = dp12.Cross(dp02);
			if (ng.LengthSquared() == 0.0f)
				return false;

			ng.Normalize();
			ng.GenerateCoordinateSpace(dpdu, dpdv);
		}

		// 计算击中位置的p和UV
		Vector3 pHit = b0 * p0 + b1 * p1 + b2 * p2;
		Vector2 uvHit = b0 * uv[0] + b1 * uv[1] + b2 * uv[2];

		outHitInfo = NXHit(pSubMesh, pHit, uvHit, -localRay.direction, dpdu, dpdv);
		outDist = t;
	}

	return true;
}

NXSubMesh::NXSubMesh(NXPrimitive* pPrimitive) :
	m_pMaterial(nullptr),
	m_parent(pPrimitive)
{
}

NXSubMesh::~NXSubMesh()
{
}

void NXSubMesh::UpdateViewParams()
{
	return m_parent->UpdateViewParams(); 
}

void NXSubMesh::Update()
{
	if (m_pMaterial)
	{
		m_pMaterial->Update();
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

bool NXSubMesh::RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist)
{
	bool bSuccess = false;

	for (UINT i = 0; i < GetFaceCount(); i++)
	{
		NXTriangle face = GetFaceTriangle(i);
		if (face.RayCast(localRay, outHitInfo, outDist))
		{
			outHitInfo.faceIndex = i;
			bSuccess = true;
		}
	}
	return bSuccess;
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

