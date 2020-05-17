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
	
	// TODO�������SRVӦ�øĳ�ȫ��ͨ�õ�
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
	p1t = Vector3::Permute(p0t, kx, ky, kz);
	p2t = Vector3::Permute(p0t, kx, ky, kz);

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
	if (outHitInfo.distance > t)
	{
		// ����dpdu��dpdv��
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
			// ���uv����ʽ���=0����ô�޷��������Чdpdu dpdv��
			// �����������Ҫǿ��Ϊ����������һ������ϵ��
			Vector3 ng = dp12.Cross(dp02);
			if (ng.LengthSquared() == 0.0f)
				return false;

			ng.Normalize();
			ng.GenerateCoordinateSpace(dpdu, dpdv);
		}

		// �������λ�õ�p��UV
		Vector3 pHit = b0 * p0 + b1 * p1 + b2 * p2;
		Vector2 uvHit = b0 * uv[0] + b1 * uv[1] + b2 * uv[2];

		�����ؿ���һ��outHitInfoӦ���ں�ʱת��������ռ䣡��
		outHitInfo.position = pHit;
		outHitInfo.primitive = pShape;
		outHitInfo.dpdu = dpdu;
		outHitInfo.dpdv = dpdv;

		// Ȼ�����hitInfo��shading���֡�
		bool bEnableNormal = true;
		bool bEnableTangent = false;	// ������ģ���Դ�����������
		bool bEnableNormalDerivative = false;	// ������dndu��dndv��

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
				// ����з��ߣ���һ������dndu dndv
				// �ͼ���dpdu dpdv�ķ�����ȫ��ͬ��Ψһ�ı仯�ǽ�λ��p�滻Ϊ����n
				Vector3 dn02 = n[0] - n[2];
				Vector3 dn12 = n[1] - n[2];
				if (uvdet >= 1e-6)
				{
					dndu = (dn02 * dv12 - dn12 * dv02) * uvdetInv;
					dndv = (dn12 * du02 - dn02 * du12) * uvdetInv;
				}

				if (uvdet < 1e-6 || dndu.Cross(dndv).LengthSquared == 0.0f)
				{
					// ���uv����ʽ���=0����ô�޷��������Чdndu dndv��
					// �����������Ҫǿ��Ϊ����������һ������ϵ��
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