#include "NXPhoton.h"
#include "NXRandom.h"
#include "NXScene.h"
#include "NXKdTree.h"
#include "NXPrimitive.h"
#include "NXIntersection.h"

NXPhotonMap::NXPhotonMap(int numPhotons) :
	m_numPhotons(numPhotons)
{
}

void NXPhotonMap::Generate(const std::shared_ptr<NXScene>& pScene, PhotonMapType photonMapType)
{
	switch (photonMapType)
	{
	case Caustic:
		GenerateCausticMap(pScene);
		break;
	case Global:
		GenerateGlobalMap(pScene);
		break;
	default:
		break;
	}
}

void NXPhotonMap::GetNearest(const Vector3& position, const Vector3& normal, float& out_distSqr, priority_queue_distance_cartesian<NXPhoton>& out_nearestPhotons, int maxPhotonsLimit, float range2, LocateFilter locateFilter)
{
	m_pKdTree->GetNearest(position, normal, out_distSqr, out_nearestPhotons, maxPhotonsLimit, range2, locateFilter);
}

void NXPhotonMap::GenerateCausticMap(const std::shared_ptr<NXScene>& pScene)
{
	std::vector<NXPhoton> causticPhotons;

	auto pLights = pScene->GetPBRLights();
	int lightCount = (int)pLights.size();
	if (!lightCount) return;
	float pdfLight = 1.0f / lightCount;

	for (int i = 0; i < m_numPhotons; i++)
	{
		int sampleLight = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);

		float pdfPos, pdfDir;
		Vector3 power;
		Ray ray;
		Vector3 lightNormal;
		Vector3 Le = pLights[sampleLight]->Emit(ray, lightNormal, pdfPos, pdfDir);

		if (pdfPos == 0.0f || pdfDir == 0.0f)
			continue;

		power = Le * fabsf(lightNormal.Dot(ray.direction)) / (pdfLight * pdfPos * pdfDir);

		int depth = 0;
		bool bIsDiffuse = true;
		bool bHasSpecularOrGlossy = false;
		while (true)
		{
			NXHit hitInfo;
			bool bIsIntersect = pScene->RayCast(ray, hitInfo);
			if (!bIsIntersect)
				break;

			hitInfo.GenerateBSDF(false);

			float pdf;
			Vector3 nextDirection;
			std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
			Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
			bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
			bHasSpecularOrGlossy |= !bIsDiffuse;
			sampleEvent.reset();

			if (f.IsZero() || pdf == 0) break;

			if (bIsDiffuse)
			{
				if (bHasSpecularOrGlossy)
				{
					// make new photon
					NXPhoton photon;
					photon.position = hitInfo.position;
					photon.direction = hitInfo.direction;
					photon.power = power;
					photon.depth = depth;
					causticPhotons.push_back(photon);
				}
				break;
			}

			depth++;
			Vector3 reflectance = f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;

			auto mat = hitInfo.pPrimitive->GetPBRMaterial();
			float random = NXRandom::GetInstance()->CreateFloat();

			// Roulette
			if (random > mat->m_probability)
				break;

			power *= reflectance / mat->m_probability;

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;
		}
	}

	m_pKdTree.reset();
	m_pKdTree = std::make_shared<NXKdTree<NXPhoton>>();
	m_pKdTree->BuildBalanceTree(causticPhotons);
}

void NXPhotonMap::GenerateGlobalMap(const std::shared_ptr<NXScene>& pScene)
{
	std::vector<NXPhoton> globalPhotons;

	auto pLights = pScene->GetPBRLights();
	int lightCount = (int)pLights.size();
	if (!lightCount) return;
	float pdfLight = 1.0f / lightCount;

	for (int i = 0; i < m_numPhotons; i++)
	{
		int sampleLight = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);

		float pdfPos, pdfDir;
		Vector3 power;
		Ray ray;
		Vector3 lightNormal;
		Vector3 Le = pLights[sampleLight]->Emit(ray, lightNormal, pdfPos, pdfDir);

		if (pdfPos == 0.0f || pdfDir == 0.0f)
			continue;

		power = Le * fabsf(lightNormal.Dot(ray.direction)) / (pdfLight * pdfPos * pdfDir);
		if (Vector3::IsNaN(power))
		{
			printf("Le: %f %f %f\n", Le.x, Le.y, Le.z);
			printf("pdfLight: %f pdfPos: %f pdfDir: %f\n", pdfLight, pdfPos, pdfDir);
		}

		int depth = 0;
		bool bIsDiffuse = false;
		bool bIsGlossy = false;
		while (true)
		{
			NXHit hitInfo;
			bool bIsIntersect = pScene->RayCast(ray, hitInfo);
			if (!bIsIntersect)
				break;

			hitInfo.GenerateBSDF(false);

			float pdf;
			Vector3 nextDirection;
			std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
			Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
			bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
			sampleEvent.reset();

			if (f.IsZero() || pdf == 0) break;

			if (bIsDiffuse)
			{
				// make new photon
				NXPhoton photon;
				photon.position = hitInfo.position;
				photon.direction = hitInfo.direction;
				photon.power = power;
				photon.depth = depth;
				globalPhotons.push_back(photon);

				depth++;
			}

			Vector3 reflectance = f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;

			auto mat = hitInfo.pPrimitive->GetPBRMaterial();
			float random = NXRandom::GetInstance()->CreateFloat();

			// Roulette
			if (random > mat->m_probability || random == 0)
				break;

			power *= reflectance / mat->m_probability;

			if (Vector3::IsNaN(power))
			{
				printf("reflectance: %f %f %f\n", reflectance.x, reflectance.y, reflectance.z);
				printf("mat->m_probability: %f\n", mat->m_probability);
				printf("f: %f %f %f\n", f.x, f.y, f.z);
				printf("hitInfo.shading.normal: %f %f %f\n", hitInfo.shading.normal.x, hitInfo.shading.normal.y, hitInfo.shading.normal.z);
				printf("nextDirection: %f %f %f\n", nextDirection.x, nextDirection.y, nextDirection.z);
				printf("pdf: %f\n", pdf);
			}

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;
		}
	}

	m_pKdTree.reset();
	m_pKdTree = std::make_shared<NXKdTree<NXPhoton>>();
	m_pKdTree->BuildBalanceTree(globalPhotons);
}
