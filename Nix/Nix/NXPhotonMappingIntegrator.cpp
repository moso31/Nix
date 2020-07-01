#include "NXPhotonMappingIntegrator.h"
#include "NXRandom.h"

NXPhotonMappingIntegrator::NXPhotonMappingIntegrator()
{
}

NXPhotonMappingIntegrator::~NXPhotonMappingIntegrator()
{
	m_photons.clear();
}

void NXPhotonMappingIntegrator::GeneratePhotons(const shared_ptr<NXScene>& pScene)
{
	m_photons.clear();

	printf("Generate photons...");
	int numPhotons = 10000;
	float numPhotonsInv = 1.0f / (float)numPhotons;
	for (int i = 0; i < numPhotons; i++)	
	{
		auto pLights = pScene->GetPBRLights();
		int lightCount = pLights.size();
		int sampleLight = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);

		float pdfLight = 1.0f / lightCount;
		float pdfPos, pdfDir;
		Vector3 throughput;
		Ray ray;
		Vector3 lightNormal;
		Vector3 Le = pLights[sampleLight]->SampleEmissionRadiance(ray, lightNormal, pdfPos, pdfDir);
		throughput = numPhotonsInv * Le * fabsf(lightNormal.Dot(ray.direction)) / (pdfLight * pdfPos * pdfDir);

		int depth = 0;
		while (true)
		{
			NXHit hitInfo;
			bool bIsIntersect = pScene->RayCast(ray, hitInfo);
			if (!bIsIntersect)
				break;

			hitInfo.ConstructReflectionModel();

			Vector3 nextDirection;
			float pdfBSDF;
			Vector3 f = hitInfo.BSDF->Sample_f(-ray.direction, nextDirection, pdfBSDF, REFLECTIONTYPE_ALL);
			if (f.IsZero() || pdfBSDF == 0) break;
			Vector3 reflectance = f * hitInfo.shading.normal.Dot(nextDirection) / pdfBSDF;

			// Roulette
			float q = max(0, 1.0f - reflectance.GetGrayValue());
			float random = NXRandom::GetInstance()->CreateFloat();
			if (random < q) break;

			throughput *= reflectance / (1 - q);

			// make new photon
			NXPhoton photon;
			photon.position = hitInfo.position;
			photon.direction = hitInfo.direction;
			photon.power = throughput;
			photon.depth = depth;
			m_photons.push_back(photon);

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;

			depth++;
		}
	}

	// use kd-tree manage all photons.
	printf("done.\n");
}

Vector3 NXPhotonMappingIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
{
	if (m_photons.empty())
	{
		printf("Error: Couldn't find photon map data!\n");
		return Vector3(0.0f);
	}

	NXHit hitInfo;
	Vector3 result;
	if (pScene->RayCast(ray, hitInfo))
	{
		Vector3 pos = hitInfo.position;

		float d = FLT_MAX;
		for (auto it = m_photons.begin(); it != m_photons.end(); it++)
		{
			float dist = Vector3::DistanceSquared(pos, it->position);
			if (dist < d)
			{
				d = dist;
				result = it->power;
			}
		}

		if (d > 0.0001f) result = Vector3(1.0f);
	}
	else
	{
		result = Vector3(1.0f);
	}

	return result;
}
