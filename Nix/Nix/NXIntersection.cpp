#include "NXIntersection.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXPBRMaterial.h"

NXHit::NXHit(NXPrimitive* pPrimitive, const Vector3& position, const Vector2& uv, const Vector3& direction, const Vector3& dpdu, const Vector3& dpdv) :
	pPrimitive(pPrimitive),
	position(position),
	uv(uv),
	direction(direction),
	dpdu(dpdu),
	dpdv(dpdv),
	normal(dpdv.Cross(dpdu)),
	faceIndex(-1),
	BSDF(nullptr)
{
	normal.Normalize();
}

NXHit::~NXHit()
{
	if (BSDF)
	{
		BSDF->Release();
		delete BSDF;
	}
}

void NXHit::GenerateBSDF(bool IsFromCamera)
{
	if (BSDF)
	{
		BSDF->Release();
		delete BSDF;
	}
	NXPBRMaterial* pMat = pPrimitive->GetPBRMaterial();
	BSDF = new NXBSDF(*this, pMat);
}

void NXHit::SetShadingGeometry(Vector3 shadingdpdu, Vector3 shadingdpdv)
{
	shading.dpdu = shadingdpdu;
	shading.dpdv = shadingdpdv;
	shading.normal = shadingdpdv.Cross(shadingdpdu);

	// ����˵���е㴦������������shading������Ӧ��ʼ�մ���ͬһ����
	// ��������������һ����ʹ��dpdu��dpdv�Ĳ������ģ���shading��������һ����������ʹ��mesh�������ݣ�
	// ���Ի�����mesh��������Ϊ׼��ԭ��Ӧ������normal��ѭshading.normal�ĳ���
	if (normal.Dot(shading.normal) < 0)
		normal = -normal;
}

void NXHit::LocalToWorld()
{
	Matrix mxWorld = pPrimitive->GetWorldMatrix();
	position = Vector3::Transform(position, mxWorld);
	Vector3::TransformNormal(direction, mxWorld).Normalize(direction);
	Vector3::TransformNormal(normal, mxWorld).Normalize(normal);
	Vector3::TransformNormal(dpdu, mxWorld).Normalize(dpdu);
	Vector3::TransformNormal(dpdv, mxWorld).Normalize(dpdv);
	Vector3::TransformNormal(shading.normal, mxWorld).Normalize(shading.normal);
	Vector3::TransformNormal(shading.dpdu, mxWorld).Normalize(shading.dpdu);
	Vector3::TransformNormal(shading.dpdv, mxWorld).Normalize(shading.dpdv);
}

void NXHit::Reset()
{
	pPrimitive = nullptr;
	if (BSDF)
	{
		BSDF->Release();
		delete BSDF;

		// Reset�Ժ�BSDFָ�����п������õģ�������Ҫ��BSDFָ���ÿգ���ȷ���´�ʹ��ʱ���ڴ氲ȫ��
		BSDF = nullptr;	
	}
}
