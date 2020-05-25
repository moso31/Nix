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
	hitInfo.BSDF = make_shared<NXBSDF>(hitInfo);
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
		// 粗糙度roughness-alpha转换。roughness=0：光滑，roughness=1：粗糙
		float alpha = NXRDistributionBeckmann::RoughnessToAlpha(Roughness);
		shared_ptr<NXRDistributionBeckmann> distrib = make_shared<NXRDistributionBeckmann>(alpha);
		shared_ptr<NXFresnel> fresnel = make_shared<NXFresnelDielectric>(1.0f, 1.5f);	// 塑料折射率1.5
		hitInfo.BSDF->AddReflectionModel(make_shared<NXRMicrofacetReflection>(Specular, distrib, fresnel));
	}
}

void NXCommonMaterial::ConstructReflectionModel(NXHit& hitInfo)
{
	hitInfo.BSDF = make_shared<NXBSDF>(hitInfo);
	Vector3 Diffuse, Specular;
	Diffuse = BaseColor * (1.0f - Metalness);
	Specular = Vector3::Lerp(Vector3(0.04), BaseColor, Metalness);	// 0.04 是拟合的。

	if (Diffuse != Vector3(0.0f))
	{
		hitInfo.BSDF->AddReflectionModel(make_shared<NXRLambertianReflection>(Diffuse));
	}
	if (Specular != Vector3(0.0f))
	{
		float alpha = Roughness;
		shared_ptr<NXRDistributionBeckmann> distrib = make_shared<NXRDistributionBeckmann>(alpha);
		shared_ptr<NXFresnelCommon> fresnel = make_shared<NXFresnelCommon>(Specular);
		hitInfo.BSDF->AddReflectionModel(make_shared<NXRMicrofacetReflection>(Specular, distrib, fresnel));
	}
}
