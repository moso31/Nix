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

void NXPhotonMap::Generate(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera, PhotonMapType photonMapType)
{
	switch (photonMapType)
	{
	case Caustic:
		GenerateCausticMap(pScene, pCamera);
		break;
	case Global:
		GenerateGlobalMap(pScene, pCamera);
		break;
	default:
		break;
	}
}

void NXPhotonMap::GetNearest(const Vector3& position, const Vector3& normal, float& out_distSqr, priority_queue_distance_cartesian<NXPhoton>& out_nearestPhotons, int maxLimit, float range, LocateFilter locateFilter)
{
	m_pKdTree->GetNearest(position, normal, out_distSqr, out_nearestPhotons, maxLimit, range, locateFilter);
}

void NXPhotonMap::GenerateCausticMap(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera)
{
	vector<NXPhoton> causticPhotons;

	printf("Generating caustic photons...");
	float numCausticPhotonsInv = 1.0f / (float)m_numPhotons;
	for (int i = 0; i < m_numPhotons; i++)
	{
		auto pLights = pScene->GetPBRLights();
		int lightCount = (int)pLights.size();
		int sampleLight = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);

		float pdfLight = 1.0f / lightCount;
		float pdfPos, pdfDir;
		Vector3 throughput;
		Ray ray;
		Vector3 lightNormal;
		Vector3 Le = pLights[sampleLight]->Emit(ray, lightNormal, pdfPos, pdfDir);
		throughput = Le * fabsf(lightNormal.Dot(ray.direction)) / (pdfLight * pdfPos * pdfDir);

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
			shared_ptr<NXBSDF::SampleEvents> sampleEvent = make_shared<NXBSDF::SampleEvents>();
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
					photon.power = throughput * numCausticPhotonsInv;
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

			throughput *= reflectance / mat->m_probability;

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;
		}
	}

	m_pKdTree.reset();
	m_pKdTree = make_shared<NXKdTree<NXPhoton>>();
	m_pKdTree->BuildBalanceTree(causticPhotons);
	printf("done.\n");
}

void NXPhotonMap::GenerateGlobalMap(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera)
{
	vector<NXPhoton> globalPhotons;

	printf("Generating global photons...");
	float numGlobalPhotonsInv = 1.0f / (float)m_numPhotons;
	for (int i = 0; i < m_numPhotons; i++)
	{
		auto pLights = pScene->GetPBRLights();
		int lightCount = (int)pLights.size();
		int sampleLight = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);

		float pdfLight = 1.0f / lightCount;
		float pdfPos, pdfDir;
		Vector3 throughput;
		Ray ray;
		Vector3 lightNormal;
		Vector3 Le = pLights[sampleLight]->Emit(ray, lightNormal, pdfPos, pdfDir);
		throughput = Le * fabsf(lightNormal.Dot(ray.direction)) / (pdfLight * pdfPos * pdfDir);

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
			shared_ptr<NXBSDF::SampleEvents> sampleEvent = make_shared<NXBSDF::SampleEvents>();
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
				photon.power = throughput * numGlobalPhotonsInv;
				photon.depth = depth;
				globalPhotons.push_back(photon);

				depth++;
			}

			Vector3 reflectance = f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;

			auto mat = hitInfo.pPrimitive->GetPBRMaterial();
			float random = NXRandom::GetInstance()->CreateFloat();

			// Roulette
			if (random > mat->m_probability)
				break;

			throughput *= reflectance / mat->m_probability;

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;
		}
	}

	m_pKdTree.reset();
	m_pKdTree = make_shared<NXKdTree<NXPhoton>>();
	m_pKdTree->BuildBalanceTree(globalPhotons);
	printf("done.\n");
}
