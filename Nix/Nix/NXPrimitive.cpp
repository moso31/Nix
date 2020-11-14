#include "NXPrimitive.h"
#include "NXScene.h"
#include "NXPBRMaterial.h"
#include "NXIntersection.h"
#include "GlobalBufferManager.h"
#include "NXRandom.h"
#include "SamplerMath.h"

using namespace SamplerMath;

NXPrimitive::NXPrimitive() :
	m_pPBRMaterial(nullptr),
	m_pTangibleLight(nullptr),
	m_bEnableNormal(true),
	m_bEnableTangent(false),
	m_bEnableNormalDerivative(false)
{
}

void NXPrimitive::Update()
{
	// 所以为什么要转置？龙书中没有这一步转置。
	// 回头看一下吧，我忘掉矩阵计算最早基于什么参考系了……
	auto mxWorld = m_worldMatrix.Transpose();
	NXGlobalBufferManager::m_cbDataObject.world = mxWorld;
	NXGlobalBufferManager::m_cbDataObject.worldInverseTranspose = mxWorld.Invert().Transpose();
	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbObject.Get(), 0, nullptr, &NXGlobalBufferManager::m_cbDataObject, 0, 0);

	if (m_pPBRMaterial)
	{
		g_pContext->UpdateSubresource(m_cbMaterial.Get(), 0, nullptr, &m_cbDataMaterial, 0, 0);
	}
}

void NXPrimitive::Render()
{
	UINT stride = sizeof(VertexPNTT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

void NXPrimitive::Release()
{
	NXObject::Release();
}

void NXPrimitive::CalculateTangents(bool bUpdateVertexIndexBuffer)
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

void NXPrimitive::SetMaterialPBR(NXPBRMaterial* mat)
{
	m_pPBRMaterial = mat;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferMaterial);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbMaterial));

	m_cbDataMaterial = m_pPBRMaterial->GetConstantBuffer();
}

NXPBRMaterial* NXPrimitive::GetPBRMaterial() const
{
	return m_pPBRMaterial;
}

AABB NXPrimitive::GetAABBWorld()
{
	AABB worldAABB;
	AABB::Transform(m_aabb, m_worldMatrix, worldAABB);
	return worldAABB;
}

AABB NXPrimitive::GetAABBLocal() const
{
	return m_aabb;
}

NXTriangle NXPrimitive::GetTriangle(int faceIndex) 
{
	return NXTriangle(this, faceIndex * 3);
}

void NXPrimitive::UpdateSurfaceAreaInfo()
{
	int faceCount = (int)m_indices.size() / 3;
	m_triangleAreas.resize(faceCount);
	for (int i = 0; i < faceCount; i++)
	{
		float lastArea = i ? m_triangleAreas[i - 1] : 0.0f;
		m_triangleAreas[i] = lastArea + GetTriangle(i).Area();
	}
	m_fArea = m_triangleAreas[faceCount - 1];
}

float NXPrimitive::GetSurfaceArea()
{
	if (m_triangleAreas.empty() && !m_vertices.empty())
		UpdateSurfaceAreaInfo();
	return m_fArea;
}

bool NXPrimitive::RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist)
{
	Ray localRay = worldRay.Transform(m_worldMatrixInv);

	// 遍历所有三角形寻找最近交点。还可以进一步优化成BVH，但暂时没做。
	bool bSuccess = false;
	int faceCount = (int)m_indices.size() / 3;
	for (int i = 0; i < faceCount; i++)
	{
		if (GetTriangle(i).RayCast(localRay, outHitInfo, outDist))
		{
			outHitInfo.faceIndex = i;
			bSuccess = true;
		}
	}

	return bSuccess;
}

void NXPrimitive::SampleForArea(Vector3& o_pos, Vector3& o_norm, float& o_pdfA)
{
	Vector2 r = NXRandom::GetInstance().CreateVector2();
	Vector2 b = UniformTriangleSample(r);	// 重心坐标
	NXTriangle tri = SampleTriangle();
	VertexPNTT P0 = tri.GetVertex(0);
	VertexPNTT P1 = tri.GetVertex(1);
	VertexPNTT P2 = tri.GetVertex(2);
	o_pos = b.x * P0.pos + b.y * P1.pos + (1 - b.x - b.y) * P2.pos;
	o_norm = (P1.pos - P2.pos).Cross(P1.pos - P0.pos);
	o_norm.Normalize();
	if (m_bEnableNormal)
	{
		Vector3 ns = b.x * P0.norm + b.y * P1.norm + (1 - b.x - b.y) * P2.norm;
		if (o_norm.Dot(ns) < 0)
			o_norm = -o_norm;
	}

	o_pos = Vector3::Transform(o_pos, m_worldMatrix);
	o_norm = Vector3::TransformNormal(o_norm, m_worldMatrix);
	o_pdfA = 1.0f / GetSurfaceArea();
}

void NXPrimitive::SampleForSolidAngle(const NXHit& hitInfo, Vector3& o_pos, Vector3& o_norm, float& o_pdfW)
{
	float pdfA;
	SampleForArea(o_pos, o_norm, pdfA);
	Vector3 dirLight = hitInfo.position - o_pos;
	float cosTheta = o_norm.Dot(dirLight);
	float dist2 = dirLight.LengthSquared();
	if (dist2 < 0.0f)
		o_pdfW = 0.0f;
	else
	{
		dirLight.Normalize();
		o_pdfW = pdfA * dist2 / fabsf(o_norm.Dot(dirLight));
	}
}

float NXPrimitive::GetPdfSolidAngle(const NXHit& hitInfo, const Vector3& posLight, const Vector3& normLight, const Vector3& dirLight)
{
	float pdfA = GetPdfArea();
	float dist2 = Vector3::DistanceSquared(posLight, hitInfo.position);
	if (dist2 < 0.0f)
		return 0.0f;

	return pdfA * dist2 / fabsf(normLight.Dot(dirLight));
}

void NXPrimitive::InitVertexIndexBuffer()
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

void NXPrimitive::InitAABB()
{
	for (auto it = m_vertices.begin(); it != m_vertices.end(); it++)
	{
		m_points.push_back(it->pos);
	}
	AABB::CreateFromPoints(m_aabb, m_points.size(), m_points.data(), sizeof(Vector3));
}

NXTriangle NXPrimitive::SampleTriangle()
{
	float randomArea = NXRandom::GetInstance().CreateFloat() * GetSurfaceArea();
	int sampleId = 0;
	for (sampleId = 0; sampleId < m_triangleAreas.size() - 1; sampleId++)
	{
		if (randomArea < m_triangleAreas[sampleId])
			break;
	}
	return GetTriangle(sampleId);
}

NXTriangle::NXTriangle(NXPrimitive* pShape, int startIndex) :
	pShape(pShape),
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
	return pShape->m_vertices[pShape->m_indices[startIndex + VertexId]];
}

bool NXTriangle::RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist)
{
	VertexPNTT data0 = pShape->m_vertices[pShape->m_indices[startIndex]];
	VertexPNTT data1 = pShape->m_vertices[pShape->m_indices[startIndex + 1]];
	VertexPNTT data2 = pShape->m_vertices[pShape->m_indices[startIndex + 2]];

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

		outHitInfo = NXHit(pShape, pHit, uvHit, -localRay.direction, dpdu, dpdv);

		Vector3 ns, ss, ts;
		Vector3 dndu, dndv;

		if (pShape->m_bEnableNormal)
		{
			Vector3 n[3];
			n[0] = data0.norm;
			n[1] = data0.norm;
			n[2] = data0.norm;

			ns = b0 * n[0] + b1 * n[1] + b2 * n[2];
			if (ns.LengthSquared() > 0)
				ns.Normalize();
			else
				ns = outHitInfo.normal;

			if (pShape->m_bEnableNormalDerivative)	// 暂不生成dndu，dndv。
			{
				// 如果有法线，进一步计算dndu dndv
				// 和计算dpdu dpdv的方法完全相同，唯一的变化是将位置p替换为法线n
				Vector3 dn02 = n[0] - n[2];
				Vector3 dn12 = n[1] - n[2];
				if (uvdet >= 1e-6)
				{
					dndu = (dn02 * dv12 - dn12 * dv02) * uvdetInv;
					dndv = (dn12 * du02 - dn02 * du12) * uvdetInv;
				}

				if (uvdet < 1e-6 || dndu.Cross(dndv).LengthSquared() == 0.0f)
				{
					// 如果uv行列式结果=0，那么无法计算出有效dndu dndv。
					// 这种情况下需要强行为切向量生成一个坐标系。
					Vector3 nt = dn12.Cross(dn02);
					if (nt.LengthSquared() == 0.0f)
						return false;

					nt.Normalize();
					nt.GenerateCoordinateSpace(dndu, dndv);
				}
			}
			else
			{
				dndu = Vector3(0.0f);
				dndv = Vector3(0.0f);
			}
		}
		else
		{
			ns = outHitInfo.normal;
			dndu = Vector3(0.0f);
			dndv = Vector3(0.0f);
		}

		if (pShape->m_bEnableTangent)		// 暂不考虑模型自带的切线数据
		{
#if _TODO_
			Vector3 s[3];
			s[0] = data0.tangent;
			s[1] = data1.tangent;
			s[2] = data2.tangent;

			ss = b0 * s[0] + b1 * s[1] + b2 * s[2];
			if (ss.LengthSquared() > 0)
				ss.Normalize();
			else
				ss = outHitInfo.dpdu;
#endif
		}
		else ss = outHitInfo.dpdu;
		
		ts = ss.Cross(ns);
		if (ts.LengthSquared() > 0.f) {
			ts.Normalize();
			ss = ns.Cross(ts);
		}
		else
			ns.GenerateCoordinateSpace(ss, ts);

		outHitInfo.SetShadingGeometry(ss, ts);
		outDist = t;
	}

	return true;
}