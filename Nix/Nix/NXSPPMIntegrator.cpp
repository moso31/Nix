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

	UINT nPixels = m_imageSize.x * m_imageSize.y;
	std::unique_ptr<NXSPPMPixel[]> pixels(new NXSPPMPixel[nPixels]);
	for (int k = 0; ; k++)
	{
		printf("SPPM iteration sequences rendering...(Image %d)\n", k);

		bool bRenderOnce = true;// k % 10 == 0;
		ImageBMPData* pImageData = nullptr;
		if (bRenderOnce)
		{
			pImageData = new ImageBMPData[nPixels];
			memset(pImageData, 0, sizeof(ImageBMPData) * nPixels);
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

				Vector3 f(0.0f);
				Vector3 Le(0.0f), Lr(0.0f);
				Vector3 throughput(1.0f);

				bool bIsIntersect = false;
				bool bIsDiffuse = false;
				while (true)
				{
					hitInfo = NXHit();
					bIsIntersect = pScene->RayCast(ray, hitInfo);
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
					Le += throughput * UniformLightOne(ray, pScene, hitInfo);

					std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
					f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
					bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
					sampleEvent.reset();

					if (bIsDiffuse) break;

					throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
					ray = Ray(hitInfo.position, nextDirection);
					ray.position += ray.direction * NXRT_EPSILON;
				}

				if (bIsDiffuse && bIsIntersect)
				{
					// multiple diffuse reflections.
					throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
					Ray nextRay = Ray(hitInfo.position, nextDirection);
					nextRay.position += nextRay.direction * NXRT_EPSILON;

					NXHit hitInfoDiffuse;
					if (pScene->RayCast(nextRay, hitInfoDiffuse))
					{
						Vector3 posDiff = hitInfoDiffuse.position;
						Vector3 normDiff = hitInfoDiffuse.shading.normal;
						
						hitInfoDiffuse.GenerateBSDF(true);

						priority_queue_distance_cartesian<NXPhoton> nearestPhotons([posDiff](const NXPhoton& photonA, const NXPhoton& photonB) {
							float distA = Vector3::DistanceSquared(posDiff, photonA.position);
							float distB = Vector3::DistanceSquared(posDiff, photonB.position);
							return distA < distB;
							});

						const static float alpha = 0.66666667f;
						UINT photons;	// 下一次累积的光子数量
						float radius2;
						if (!k)
						{
							photons = 100;
							m_pPhotonMap->GetNearest(posDiff, normDiff, radius2, nearestPhotons, photons, FLT_MAX, LocateFilter::Sphere);
						}
						else
						{
							m_pPhotonMap->GetNearest(posDiff, normDiff, radius2, nearestPhotons, -1, pixel.radius2, LocateFilter::Sphere);
							photons = pixel.photons + (UINT)(alpha * nearestPhotons.size());
							radius2 = pixel.radius2 * ((float)photons / (pixel.photons + (UINT)nearestPhotons.size()));
						}

						Vector3 flux(0.0f);
						while (!nearestPhotons.empty())
						{
							auto photon = nearestPhotons.top();
							Vector3 f = hitInfoDiffuse.BSDF->Evaluate(-nextRay.direction, photon.direction, pdf);
							flux += f * photon.power;
							nearestPhotons.pop();
						}

						if (k) flux = (pixel.flux + flux) * radius2 / pixel.radius2;

						Lr = throughput * flux / (XM_PI * (float)m_pPhotonMap->GetPhotonCount() * (k + 1) * radius2);
						pixel.flux = flux;
						pixel.photons = photons;
						pixel.radius2 = radius2;
						pixel.Ld += Le;
					}

					if (bRenderOnce)
					{
						Vector3 result = (pixel.Ld / (float)(k + 1)) + Lr;

						XMINT3 RGBValue(
							result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
							result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
							result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

						int rgbIdx = (m_imageSize.y - j - 1) * m_imageSize.x + i;
						pImageData[rgbIdx].r = RGBValue.x;
						pImageData[rgbIdx].g = RGBValue.y;
						pImageData[rgbIdx].b = RGBValue.z;
					}
				}
			}

			printf("\r%.2f%% ", (float)++m_progress * 100.0f / (float)m_imageSize.x);	// 进度条更新
		}

		m_outFilePath = "D:\\Nix_SPPM_" + std::to_string(k) + ".bmp";
		if (bRenderOnce)
		{
			ImageGenerator::GenerateImageBMP((byte*)pImageData, m_imageSize.x, m_imageSize.y, m_outFilePath.c_str());
			delete pImageData;
		}
		printf("done.\n");
	}
}

void NXSPPMIntegrator::RefreshPhotonMap(const std::shared_ptr<NXScene>& pScene)
{
	m_pPhotonMap.reset();
	m_pPhotonMap = std::make_shared<NXPhotonMap>(m_numPhotonsEachStep);
	m_pPhotonMap->Generate(pScene, PhotonMapType::Global);
}