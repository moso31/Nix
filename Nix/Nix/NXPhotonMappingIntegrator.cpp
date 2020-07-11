#include "NXPhotonMappingIntegrator.h"
#include "NXRandom.h"
#include "NXCamera.h"
#include "SamplerMath.h"

NXPhotonMappingIntegrator::NXPhotonMappingIntegrator()
{
}

NXPhotonMappingIntegrator::~NXPhotonMappingIntegrator()
{
}

void NXPhotonMappingIntegrator::GeneratePhotons(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera)
{
	vector<NXPhoton> photons;

	printf("Generate photons...");
	int numPhotons = 10000000;
	float numPhotonsInv = 1.0f / (float)numPhotons;
	for (int i = 0; i < numPhotons; i++)
	{
		auto pLights = pScene->GetPBRLights();
		int lightCount = (int)pLights.size();
		int sampleLight = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);

		float pdfLight = 1.0f / lightCount;
		float pdfPos, pdfDir;
		Vector3 throughput;
		Ray ray;
		Vector3 lightNormal;
		Vector3 Le = pLights[sampleLight]->SampleEmissionRadiance(ray, lightNormal, pdfPos, pdfDir);
		throughput = Le * fabsf(lightNormal.Dot(ray.direction)) / (pdfLight * pdfPos * pdfDir);

		int depth = 0;
		bool bIsSpecular = false;
		int maxDepth = 5;
		while (depth < maxDepth)
		{
			NXHit hitInfo;
			bool bIsIntersect = pScene->RayCast(ray, hitInfo);
			if (!bIsIntersect)
				break;

			hitInfo.ConstructReflectionModel(false);

			Vector3 nextDirection;
			float pdfBSDF;
			shared_ptr<ReflectionType> outReflectType = make_shared<ReflectionType>();
			Vector3 f = hitInfo.BSDF->Sample_f(-ray.direction, nextDirection, pdfBSDF, REFLECTIONTYPE_ALL, outReflectType);
			bIsSpecular = (*outReflectType & REFLECTIONTYPE_SPECULAR);
			outReflectType.reset();

			if (f.IsZero() || pdfBSDF == 0) break;

			if (!bIsSpecular)
			{
				// 【这里的光子记录位置是否正确？是应该在吞吐量之前还是之后？】
				// make new photon
				NXPhoton photon;
				photon.position = hitInfo.position;
				photon.direction = hitInfo.direction;
				photon.power = throughput * fabsf(hitInfo.shading.normal.Dot(hitInfo.direction)) * numPhotonsInv;
				photon.depth = depth;
				m_photons.push_back(photon);

				//printf("%f %f %f\n", photon.power.x, photon.power.y, photon.power.z);

				Vector3 reflectance = f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdfBSDF;

				// Roulette
				float q = max(0, 1.0f - reflectance.GetGrayValue());
				float random = NXRandom::GetInstance()->CreateFloat();
				if (random < q) break;

				throughput *= reflectance / (1 - q);

				depth++;
			}

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;
		}
	}

	m_pKdTree.reset();
	m_pKdTree = make_shared<NXKdTree>();
	m_pKdTree->BuildBalanceTree(m_photons);
	m_photons.clear();
	// use kd-tree manage all photons.
	printf("done.\n");
}

Vector3 NXPhotonMappingIntegrator::Radiance(const Ray& cameraRay, const shared_ptr<NXScene>& pScene, int depth)
{
	if (!m_pKdTree)
	{
		printf("Error: Couldn't find photon map data!\n");
		return Vector3(0.0f);
	}

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf;
	NXHit hitInfo;
	bool bIsHit = true;

	Vector3 throughput(1.0f);
	while(bIsHit)
	{
		hitInfo = NXHit();
		bIsHit = pScene->RayCast(ray, hitInfo);
		if (bIsHit)
		{
			hitInfo.ConstructReflectionModel(true);

			Vector3 f = hitInfo.BSDF->Sample_f(-ray.direction, nextDirection, pdf, ReflectionType(REFLECTIONTYPE_REFLECTION | REFLECTIONTYPE_TRANSMISSION | REFLECTIONTYPE_SPECULAR));
			if (f.IsZero()) break;
			throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON; 
		}
	}

	Vector3 L(0.0f);
	if (!bIsHit) return L;

	Vector3 pos = hitInfo.position;

	float distSqr;
	// 大根堆，负责记录pos周围的最近顶点。
	priority_quque_NXPhoton nearestPhotons([pos](NXPhoton* photonA, NXPhoton* photonB) {
		float distA = Vector3::DistanceSquared(pos, photonA->position);
		float distB = Vector3::DistanceSquared(pos, photonB->position);
		return distA < distB;
		});

	m_pKdTree->GetNearest(pos, distSqr, nearestPhotons, -1, 0.001);

	//if (!nearestPhotons.empty()) return nearestPhotons.top()->power;	// photon data only.

	hitInfo.ConstructReflectionModel(true);

	Vector3 wo = -ray.direction;
	float photonCount = (float)nearestPhotons.size();
	if (!photonCount) 
		return L;

	float radius2 = Vector3::DistanceSquared(pos, nearestPhotons.top()->position);
	//printf("radius2 %f\n", radius2);
	Vector3 result(0.0f);
	while (!nearestPhotons.empty())
	{
		auto photon = nearestPhotons.top();
		float pdfBSDF = hitInfo.BSDF->Pdf(photon->direction, wo);
		Vector3 f = hitInfo.BSDF->f(photon->direction, wo);
		result += f * photon->power;

		//float dist = Vector3::Distance(pos, photon->position);
		//float kernelFactor = SamplerMath::EpanechnikovKernel(dist * maxDistInv);
		//result += kernelFactor * photon->power * hitInfo.BSDF->f(photon->direction, wo);

		//printf("f: %f %f %f \t", f.x, f.y, f.z);
		//printf("power: %f %f %f\n", photon->power.x, photon->power.y, photon->power.z);

		nearestPhotons.pop();
	}
	L = throughput * result / (XM_PI * radius2);
	//printf("%f %f %f\n", result.x, result.y, result.z);

	return L;
}
