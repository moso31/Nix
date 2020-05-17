#include "NXPrimitive.h"
#include "NXMaterial.h"
#include "NXPBRMaterial.h"
#include "NXIntersection.h"
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

shared_ptr<NXPBRMaterial> NXPrimitive::GetPBRMaterial() const
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

bool NXPrimitive::RayCast(const Ray& localRay, NXHit& outHitInfo)
{
	for (int i = 0; i < (int)m_indices.size() / 3; i++)
	{
		Vector3 p0 = m_vertices[m_indices[i * 3 + 0]].pos;
		Vector3 p1 = m_vertices[m_indices[i * 3 + 1]].pos;
		Vector3 p2 = m_vertices[m_indices[i * 3 + 2]].pos;

		NXTriangle triangle(dynamic_pointer_cast<NXPrimitive>(shared_from_this()), i * 3);
		triangle.RayCast(localRay, outHitInfo);
	}
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
	//NX::ThrowIfFailed(CreateWICTextureFromFile(g_pDevice, L"D:\\rgb.bmp", nullptr, &m_pTextureSRV));
}

void NXPrimitive::InitAABB()
{
	for (auto it = m_vertices.begin(); it != m_vertices.end(); it++)
	{
		m_points.push_back(it->pos);
	}
	AABB::CreateFromPoints(m_aabb, m_points.size(), m_points.data(), sizeof(Vector3));
}

NXTriangle::NXTriangle(const shared_ptr<NXPrimitive>& pShape, int startIndex) :
	p0(pShape->m_vertices[startIndex].pos),
	p1(pShape->m_vertices[startIndex + 1].pos),
	p2(pShape->m_vertices[startIndex + 2].pos)
{
}

bool NXTriangle::RayCast(const Ray& localRay, NXHit& outHitInfo)
{
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
	p1t = Vector3::Permute(p0t, kx, ky, kz);
	p2t = Vector3::Permute(p0t, kx, ky, kz);

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
	if (outHitInfo.distance > t)
	{
		// 计算dpdu和dpdv。
		Vector3 dpdu, dpdv;
		Vector2 uv[3];
		for (int i = 0; i < 3; i++)
			uv[i] = pShape->m_vertices[i].tex;

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
		if (uvdet >= 1e-6)
		{
			uvdetInv = 1.0f / uvdet;
			dpdu = (dp02 * dv12 - dp12 * dv02) * uvdetInv;
			dpdv = (dp12 * du02 - dp02 * du12) * uvdetInv;
		}

		if (uvdet < 1e-6 || dpdu.Cross(dpdv).LengthSquared == 0.0f)
		{
			// 如果uv行列式结果=0，那么无法计算出有效dpdu dpdv。
			// 这种情况下需要强行为法向量生成一个坐标系。
			Vector3 ng = dp12.Cross(dp02);
			if (ng.LengthSquared() == 0.0f)
				return false;

			ng.Normalize();
			ng.GenerateCoordinateSpace(dpdu, dpdv);
		}

		// 计算击中位置的p和UV
		Vector3 pHit = b0 * p0 + b1 * p1 + b2 * p2;
		Vector2 uvHit = b0 * uv[0] + b1 * uv[1] + b2 * uv[2];

		【慎重考虑一下outHitInfo应该在何时转换到世界空间！】
		outHitInfo.position = pHit;
		outHitInfo.primitive = pShape;
		outHitInfo.dpdu = dpdu;
		outHitInfo.dpdv = dpdv;

		// 然后更新hitInfo的shading部分。
		bool bEnableNormal = true;
		bool bEnableTangent = false;	// 不考虑模型自带的切线数据
		bool bEnableNormalDerivative = false;	// 不生成dndu，dndv。

		Vector3 ns, ss, ts;
		Vector3 dndu, dndv;

		if (bEnableNormal)
		{
			Vector3 n[3];
			n[0] = pShape->m_vertices[0].norm;
			n[1] = pShape->m_vertices[1].norm;
			n[2] = pShape->m_vertices[2].norm;

			ns = b0 * n[0] + b1 * n[1] + b2 * n[2];
			if (ns.LengthSquared() > 0)
				ns.Normalize();
			else
				ns = outHitInfo.normal;

			if (bEnableNormalDerivative)
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

				if (uvdet < 1e-6 || dndu.Cross(dndv).LengthSquared == 0.0f)
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

		if (bEnableTangent)
		{
#if _TODO_
			Vector3 s[3];
			s[0] = pShape->m_vertices[0].tangent;
			s[1] = pShape->m_vertices[1].tangent;
			s[2] = pShape->m_vertices[2].tangent;

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
			ss = ts.Cross(ns);
		}
		else
			ns.GenerateCoordinateSpace(ss, ts);

		outHitInfo.SetShadingGeometry();

		// Ensure correct orientation of the geometric normal ???
		.....

		outHitInfo.distance = t;
	}


	return true;
}