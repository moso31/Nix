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
	for (int i = 0; i < 10000; i++)	
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
		throughput = Le * fabsf(lightNormal.Dot(ray.direction)) / (pdfLight * pdfPos * pdfDir);

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
	pScene->RayCast(ray, hitInfo);

	Vector3 pos = hitInfo.position;

	vector<NXPhoton> p(m_photons);
	sort(p.begin(), p.end(), [pos](NXPhoton& photonA, NXPhoton& photonB) {
		float distA = Vector3::DistanceSquared(pos, photonA.position);
		float distB = Vector3::DistanceSquared(pos, photonB.position);
		return distA < distB;
		});

	Vector3 result;
	for (int i = 0; i < max(1, m_photons.size()); i++)
	{
		result += m_photons[i].power;
	}

	return result / 10000000;
}
