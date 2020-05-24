#include "NXPBRMaterial.h"
#include "NXReflectionModel.h"
#include "NXBSDF.h"
#include "NXFresnel.h"
#include "NXDistribution.h"

void NXMatteMaterial::ConstructReflectionModel(NXHit& hitInfo)
{
	hitInfo.BSDF = make_shared<NXBSDF>(hitInfo);
	if (Diffuse != Vector3(0.0f))
	{
		hitInfo.BSDF->AddReflectionModel(make_shared<NXRLambertianReflection>(Diffuse));
	}
}

void NXMirrorMaterial::ConstructReflectionModel(NXHit& hitInfo)
{
	hitInfo.BSDF = make_shared<NXBSDF>(hitInfo);
	shared_ptr<NXFresnel> fresnel = make_shared<NXFresnelNoOp>();
	if (Diffuse != Vector3(0.0f))
	{
		hitInfo.BSDF->AddReflectionModel(make_shared<NXRPrefectReflection>(Diffuse, fresnel));
	}
}

void NXGlassMaterial::ConstructReflectionModel(NXHit& hitInfo)
{
	hitInfo.BSDF = make_shared<NXBSDF>(hitInfo, IOR);
	shared_ptr<NXFresnel> fresnel = make_shared<NXFresnelDielectric>(1.0f, IOR);
	if (Diffuse != Vector3(0.0f))
	{
		hitInfo.BSDF->AddReflectionModel(make_shared<NXRPrefectReflection>(Diffuse, fresnel));
		hitInfo.BSDF->AddReflectionModel(make_shared<NXRPrefectTransmission>(Diffuse, 1.0f, IOR));
	}
}

void NXPlasticMaterial::ConstructReflectionModel(NXHit& hitInfo)
{
	hitInfo.BSDF = make_shared<NXBSDF>(hitInfo);
	if (Diffuse != Vector3(0.0f))
	{
		hitInfo.BSDF->AddReflectionModel(make_shared<NXRLambertianReflection>(Diffuse));
	}
	if (Specular != Vector3(0.0f))
	{
		// �ֲڶ�roughness-alphaת����roughness=0���⻬��roughness=1���ֲ�
		float alpha = NXRDistributionBeckmann::RoughnessToAlpha(Roughness);
		shared_ptr<NXRDistributionBeckmann> distrib = make_shared<NXRDistributionBeckmann>(alpha);
		shared_ptr<NXFresnel> fresnel = make_shared<NXFresnelDielectric>(1.0f, 1.5f);	// ����������1.5
		hitInfo.BSDF->AddReflectionModel(make_shared<NXRMicrofacetReflection>(Specular, distrib, fresnel));
	}
}