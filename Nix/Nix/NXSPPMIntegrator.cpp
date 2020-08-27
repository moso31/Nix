#include "NXSPPMIntegrator.h"
#include <vector>
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"

NXSPPMIntegrator::NXSPPMIntegrator(const XMINT2& imageSize, std::string outPath, UINT nPhotons) :
	NXIntegrator(imageSize),
	m_numPhotonsEachStep(nPhotons)
{
}

void NXSPPMIntegrator::Render(const std::shared_ptr<NXScene>& pScene)
{
	printf("Building BVH Trees...");
	pScene->BuildBVHTrees(HLBVH);
	printf("Done.\n");

	for (int k = 0; ; k++)
	{
		printf("SPPM iteration sequences rendering...(Image %d)\n", k);
		UINT nPixels = m_imageSize.x * m_imageSize.y;
		std::unique_ptr<NXSPPMPixel[]> pixels(new NXSPPMPixel[nPixels]);

		ImageBMPData* pImageData = new ImageBMPData[nPixels];
		memset(pImageData, 0, sizeof(ImageBMPData) * nPixels);

		for (int i = 0; i < nPixels; i++)
		{
			// 如果需要visible points初始化，写在这里
		}

		printf("Refreshing photon map...");
		RefreshPhotonMap(pScene);
		printf("done.\n");

		m_progress = 0;
		printf("Rendering...");
#pragma omp parallel for
		for (int i = 0; i < m_imageSize.x; i++)
		{
			for (int j = 0; j < m_imageSize.y; j++)
			{
				Vector2 pixelCoord((float)i, (float)j);
				Vector2 sampleCoord = pixelCoord + NXRandom::GetInstance()->CreateVector2();
				Ray ray = pScene->GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));

				UINT pixelOffset = (m_imageSize.y - j - 1) * m_imageSize.x + i;
				NXSPPMPixel& pixel = pixels[pixelOffset];

				Vector3 nextDirection;
				float pdf;
				NXHit hitInfo;

				Vector3 Le(0.0f);
				Vector3 throughput(1.0f);

				bool bIsDiffuse = false;
				while (true)
				{
					hitInfo = NXHit();
					bool bIsIntersect = pScene->RayCast(ray, hitInfo);
					if (!bIsIntersect)
						break;

					std::shared_ptr<NXPBRAreaLight> pHitAreaLight;
					if (hitInfo.pPrimitive)pHitAreaLight = hitInfo.pPrimitive->GetTangibleLight();
					else if (pScene->GetCubeMap()) pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();
					if (pHitAreaLight)
					{
						Le += throughput * pHitAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, -ray.direction);
					}

					hitInfo.GenerateBSDF(true);
					std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
					Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
					bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
					sampleEvent.reset();

					if (bIsDiffuse) break;

					throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
					ray = Ray(hitInfo.position, nextDirection);
					ray.position += ray.direction * NXRT_EPSILON;
				}

				Vector3 pos = hitInfo.position;
				Vector3 norm = hitInfo.shading.normal;

				// 大根堆，负责记录pos周围的最近顶点。
				priority_queue_distance_cartesian<NXPhoton> nearestPhotons([pos](const NXPhoton& photonA, const NXPhoton& photonB) {
					float distA = Vector3::DistanceSquared(pos, photonA.position);
					float distB = Vector3::DistanceSquared(pos, photonB.position);
					return distA < distB;
					});

				float radius2;
				m_pPhotonMap->GetNearest(pos, norm, radius2, nearestPhotons, 100, FLT_MAX, LocateFilter::Disk);

				Vector3 flux(0.0f);
				while (!nearestPhotons.empty())
				{
					auto photon = nearestPhotons.top();
					Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon.direction, pdf);
					flux += f * photon.power;
					nearestPhotons.pop();
				}

				Vector3 Lr = throughput * flux / (XM_PI * (float)m_pPhotonMap->GetPhotonCount() * radius2);

				Vector3 result = Le + Lr;

				XMINT3 RGBValue(
					result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
					result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
					result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

				int rgbIdx = (m_imageSize.y - j - 1) * m_imageSize.x + i;
				pImageData[rgbIdx].r = RGBValue.x;
				pImageData[rgbIdx].g = RGBValue.y;
				pImageData[rgbIdx].b = RGBValue.z;
			}

			printf("\r%.2f%% ", (float)++m_progress * 100.0f / (float)m_imageSize.x);	// 进度条更新
		}

		m_outFilePath = "D:\\Nix_SPPM_" + std::to_string(k) + ".bmp";
		ImageGenerator::GenerateImageBMP((byte*)pImageData, m_imageSize.x, m_imageSize.y, m_outFilePath.c_str());
		delete pImageData;
		printf("done.\n");
	}
}

void NXSPPMIntegrator::RefreshPhotonMap(const std::shared_ptr<NXScene>& pScene)
{
	m_pPhotonMap.reset();
	m_pPhotonMap = std::make_shared<NXPhotonMap>(200000);
	m_pPhotonMap->Generate(pScene, PhotonMapType::Global);
}