#include "NXPhoton.h"
#include "NXRandom.h"
#include "NXScene.h"
#include "NXKdTree.h"
#include "NXPrimitive.h"
#include "NXIntersection.h"
#include "NXCamera.h"
#include "ImageGenerator.h"

NXPhotonMap::NXPhotonMap(int numPhotons) :
	m_numPhotons(numPhotons),
	m_pKdTree(nullptr)
{
}

void NXPhotonMap::Generate(NXScene* pScene, PhotonMapType photonMapType)
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

void NXPhotonMap::Render(NXScene* pScene, const XMINT2& imageSize, std::string outFilePath)
{
	UINT nPixels = imageSize.x * imageSize.y;
	ImageBMPData* pImageData = new ImageBMPData[nPixels];
	memset(pImageData, 0, sizeof(ImageBMPData) * nPixels);

	NXCamera* pCamera = pScene->GetMainCamera();
	Vector3 camPos = pCamera->GetTranslation();
	for (auto photon : m_pData)
	{
		Vector3 camDirView = Vector3::TransformNormal(photon.position - camPos, pCamera->GetViewMatrix());
		float tx = camDirView.x * pCamera->GetProjectionMatrix()._11 / camDirView.z;
		float ty = camDirView.y * pCamera->GetProjectionMatrix()._22 / camDirView.z;
		if (fabsf(tx) >= 1.0f || fabsf(ty) >= 1.0f)
			continue;

		int x = (int)((tx + 1.0f) * 0.5f * (float)imageSize.x);
		int y = (int)((1.0f - ty) * 0.5f * (float)imageSize.y);

		int rgbIdx = (imageSize.y - y - 1) * imageSize.x + x;
		pImageData[rgbIdx].r = 255;
		pImageData[rgbIdx].g = 255;
		pImageData[rgbIdx].b = 255;
	}

	ImageGenerator::GenerateImageBMP((byte*)pImageData, imageSize.x, imageSize.y, outFilePath.c_str());
	SafeDelete(pImageData);
}

void NXPhotonMap::Release()
{
	SafeRelease(m_pKdTree);
}

void NXPhotonMap::GenerateCausticMap(NXScene* pScene)
{
	auto pLights = pScene->GetPBRLights();
	int lightCount = (int)pLights.size();
	if (!lightCount) return;
	float pdfLight = 1.0f / lightCount;

	for (int i = 0; i < m_numPhotons; i++)
	{
		int sampleLight = NXRandom::GetInstance().CreateInt(0, lightCount - 1);

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
			NXBSDF::SampleEvents* sampleEvent = new NXBSDF::SampleEvents();
			Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
			bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
			bHasSpecularOrGlossy |= !bIsDiffuse;
			SafeDelete(sampleEvent);

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
					m_pData.push_back(photon);
				}
				break;
			}

			depth++;
			Vector3 reflectance = f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;

			auto mat = hitInfo.pPrimitive->GetPBRMaterial();
			float random = NXRandom::GetInstance().CreateFloat();

			// Roulette
			if (random > mat->m_albedo.GetGrayValue())
				break;

			power *= reflectance / mat->m_albedo.GetGrayValue();

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;
		}
	}

	SafeRelease(m_pKdTree);
	m_pKdTree = new NXKdTree<NXPhoton>();
	m_pKdTree->BuildBalanceTree(m_pData);
}

void NXPhotonMap::GenerateGlobalMap(NXScene* pScene)
{
	auto pLights = pScene->GetPBRLights();
	int lightCount = (int)pLights.size();
	if (!lightCount) return;
	float pdfLight = 1.0f / lightCount;

	for (int i = 0; i < m_numPhotons; i++)
	{
		int sampleLight = NXRandom::GetInstance().CreateInt(0, lightCount - 1);

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
			NXBSDF::SampleEvents* sampleEvent = new NXBSDF::SampleEvents();
			Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
			bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
			SafeDelete(sampleEvent);

			if (f.IsZero() || pdf == 0) break;

			if (bIsDiffuse)
			{
				// make new photon
				NXPhoton photon;
				photon.position = hitInfo.position;
				photon.direction = hitInfo.direction;
				photon.power = power;
				photon.depth = depth;
				m_pData.push_back(photon);

				depth++;
			}

			Vector3 reflectance = f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;

			auto mat = hitInfo.pPrimitive->GetPBRMaterial();
			float random = NXRandom::GetInstance().CreateFloat();

			// Roulette
			if (random > mat->m_albedo.GetGrayValue() || random == 0)
				break;

			power *= reflectance / mat->m_albedo.GetGrayValue();

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;
		}
	}

	m_pKdTree = new NXKdTree<NXPhoton>();
	m_pKdTree->BuildBalanceTree(m_pData);
}
