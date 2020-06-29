#include "NXPhotonMappingIntegrator.h"
#include "NXRandom.h"

void NXPhotonMappingIntegrator::GeneratePhotons(const shared_ptr<NXScene>& pScene)
{
	auto pLights = pScene->GetPBRLights();
	int lightCount = pLights.size();
	int sampleLight = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);

	float pdfLight = 1.0f / lightCount;
	float pdfPos, pdfDir;
	Vector3 throughput;
	pLights[sampleLight]->SampleEmissionRadiance();
	
	Ray ray;
	while (true)
	{
		NXHit hitInfo;
		bool bIsIntersect = pScene->RayCast(ray, hitInfo);
		if (bIsIntersect)
		{
			hitInfo.ConstructReflectionModel();

			Vector3 nextDirection;
			float pdfBSDF;
			Vector3 f = hitInfo.BSDF->Sample_f(-ray.direction, nextDirection, pdfBSDF, REFLECTIONTYPE_ALL);
			Vector3 reflectance = f * hitInfo.normal.Dot(nextDirection) / pdfBSDF;

			// Roulette
			float q = NXRandom::GetInstance()->CreateFloat();
			if (q < 1.0f - reflectance.GetGrayValue()) break;
			throughput *= reflectance / (1 - q);

			// make new photon
			NXPhoton photon;
			photon.position = hitInfo.position;
			photon.direction = hitInfo.direction;
			photon.power = throughput;
			m_photons.push_back(photon);

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;
		}
	}
}
