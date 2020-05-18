#include "NXIntersection.h"
#include "NXScene.h"
#include "NXPBRMaterial.h"

#include "NXPrimitive.h"

NXHit::NXHit(const shared_ptr<NXPrimitive>& pPrimitive, const Vector3& position, const Vector2& uv, const Vector3& dpdu, const Vector3& dpdv) :
	pPrimitive(pPrimitive),
	position(position),
	uv(uv),
	dpdu(dpdu),
	dpdv(dpdv),
	normal(dpdu.Cross(dpdv))
{
}

void NXHit::ConstructReflectionModel()
{
	shared_ptr<NXPBRMaterial> pMat = pPrimitive->GetPBRMaterial();
	if (pMat)
		pMat->ConstructReflectionModel(shared_from_this());
}

void NXHit::SetShadingGeometry(Vector3 shadingdpdu, Vector3 shadingdpdv)
{
	shading.dpdu = shadingdpdu;
	shading.dpdv = shadingdpdv;
	shading.normal = shadingdpdu.Cross(shadingdpdv);

	// ����˵���е㴦������������shading������Ӧ��ʼ�մ���ͬһ����
	// ��������������һ����ʹ��dpdu��dpdv�Ĳ������ģ���shading��������һ����������ʹ��mesh�������ݣ�
	// ���Ի�����mesh��������Ϊ׼��ԭ��Ӧ������normal��ѭshading.normal�ĳ���
	if (normal.Dot(shading.normal) < 0)
		normal = -normal;
}
