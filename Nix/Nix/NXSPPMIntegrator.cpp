#include "NXSPPMIntegrator.h"
#include <vector>
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"

NXSPPMIntegrator::NXSPPMIntegrator(const XMINT2& imageSize, std::string outPath, UINT nCausticPhotons, UINT nGlobalPhotons) :
	NXIntegrator(imageSize),
	m_numCausticPhotonsEachStep(nCausticPhotons),
	m_numGlobalPhotonsEachStep(nGlobalPhotons)
{
}

void NXSPPMIntegrator::Render(const std::shared_ptr<NXScene>& pScene)
{
	bool bSplitRender = true;

	printf("Building BVH Trees...");
	pScene->BuildBVHTrees(HLBVH);
	printf("Done.\n");

	UINT nPixels = m_imageSize.x * m_imageSize.y;
	std::unique_ptr<NXSPPMPixel[]> pixels(new NXSPPMPixel[nPixels]);
	for (int k = 0; ; k++)
	{
		printf("SPPM iteration sequences rendering...(Image %d)\n", k);

		bool bRenderOnce = (k < 30) || (k % 10 == 0);
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

		bSplitRender ? 
			RenderWithPMSplit(pScene, pixels, pImageData, k, bRenderOnce) :
			RenderWithPM(pScene, pixels, pImageData, k, bRenderOnce);

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
	m_pCausticPhotonMap.reset();
	m_pCausticPhotonMap = std::make_shared<NXPhotonMap>(m_numCausticPhotonsEachStep);
	m_pCausticPhotonMap->Generate(pScene, PhotonMapType::Caustic);
	m_pGlobalPhotonMap.reset();
	m_pGlobalPhotonMap = std::make_shared<NXPhotonMap>(m_numGlobalPhotonsEachStep);
	m_pGlobalPhotonMap->Generate(pScene, PhotonMapType::Global);
}

void NXSPPMIntegrator::RenderWithPM(const std::shared_ptr<NXScene>& pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int depth, bool RenderOnce)
{
#pragma omp parallel for
	for (int i = 0; i < m_imageSize.x; i++)
	{
		for (int j = 0; j < m_imageSize.y; j++)
		{
			Vector2 pixelCoord((float)i, (float)j);
			Vector2 sampleCoord = pixelCoord + NXRandom::GetInstance()->CreateVector2();
			Ray ray = pScene->GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));

			UINT pixelOffset = (m_imageSize.y - j - 1) * m_imageSize.x + i;
			NXSPPMPixel& pixel = oPixels[pixelOffset];

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
				if (hitInfo.pPrimitive) pHitAreaLight = hitInfo.pPrimitive->GetTangibleLight();
				else if (pScene->GetCubeMap()) pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();
				if (pHitAreaLight)
				{
					Le += throughput * pHitAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, -ray.direction);
				}

				hitInfo.GenerateBSDF(true);

				std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
				f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
				bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
				sampleEvent.reset();

				if (f.IsZero() || pdf == 0) break;
				if (bIsDiffuse) break;

				throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
				ray = Ray(hitInfo.position, nextDirection);
				ray.position += ray.direction * NXRT_EPSILON;
			}

			if (f.IsZero() || pdf == 0)
				continue;

			if (bIsIntersect)
			{
				Vector3 posDiff = hitInfo.position;
				Vector3 normDiff = hitInfo.shading.normal;

				hitInfo.GenerateBSDF(true);

				priority_queue_distance_cartesian<NXPhoton> nearestPhotons([posDiff](const NXPhoton& photonA, const NXPhoton& photonB) {
					float distA = Vector3::DistanceSquared(posDiff, photonA.position);
					float distB = Vector3::DistanceSquared(posDiff, photonB.position);
					return distA < distB;
					});

				const static float alpha = 0.66666667f;
				UINT photons;	// 下一次累积的光子数量
				float radius2;
				if (!pixel.estimateFlag)
				{
					photons = 100;
					m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, radius2, nearestPhotons, photons, FLT_MAX, LocateFilter::Sphere);
					pixel.estimateFlag = true;
				}
				else
				{
					m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, radius2, nearestPhotons, -1, pixel.globalRadius2, LocateFilter::Sphere);
					photons = pixel.globalPhotons+ (UINT)(alpha * nearestPhotons.size());
					radius2 = nearestPhotons.empty() ?
						pixel.globalRadius2 :
						pixel.globalRadius2 * ((float)photons / (pixel.globalPhotons + (UINT)nearestPhotons.size()));
				}

				Vector3 flux(0.0f);
				while (!nearestPhotons.empty())
				{
					auto photon = nearestPhotons.top();
					Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon.direction, pdf);
					if (!f.IsZero() && pdf != 0) 
						flux += f * photon.power;
					nearestPhotons.pop();
				}
				flux *= throughput;

				if (depth) flux = (pixel.globalFlux + flux) * radius2 / pixel.globalRadius2;

				Lr = flux / (XM_PI * (float)m_pGlobalPhotonMap->GetPhotonCount() * (depth + 1) * radius2);

				pixel.globalFlux = flux;
				pixel.globalPhotons = photons;
				pixel.globalRadius2 = radius2;
				pixel.radiance += Le;

				if (RenderOnce)
				{
					Vector3 result = (pixel.radiance / (float)(depth + 1)) + Lr;

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
}

void NXSPPMIntegrator::RenderWithPMSplit(const std::shared_ptr<NXScene>& pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int depth, bool RenderOnce)
{
#pragma omp parallel for
	for (int i = 0; i < m_imageSize.x; i++)
	{
		for (int j = 0; j < m_imageSize.y; j++)
		{
			Vector2 pixelCoord((float)i, (float)j);
			Vector2 sampleCoord = pixelCoord + NXRandom::GetInstance()->CreateVector2();
			Ray ray = pScene->GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));

			UINT pixelOffset = (m_imageSize.y - j - 1) * m_imageSize.x + i;
			NXSPPMPixel& pixel = oPixels[pixelOffset];

			if (!depth)
			{
				pixel.causticRadius2 = 1e-7f;
				pixel.globalRadius2 = 1e-7f;
			}

			Vector3 nextDirection;
			float pdf, pdfPhotons;
			NXHit hitInfo;

			Vector3 f(0.0f);
			Vector3 Le(0.0f), Ld(0.0f), LrCaustic(0.0f), LrGlobal(0.0f);
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
				if (hitInfo.pPrimitive) pHitAreaLight = hitInfo.pPrimitive->GetTangibleLight();
				else if (pScene->GetCubeMap()) pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();
				if (pHitAreaLight)
				{
					Le += throughput * pHitAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, -ray.direction);
				}

				hitInfo.GenerateBSDF(true);
				Ld += throughput * UniformLightOne(ray, pScene, hitInfo);

				std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
				f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
				bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
				sampleEvent.reset();

				if (f.IsZero() || pdf == 0) break;
				if (bIsDiffuse) break;

				throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
				if (Vector3::IsNaN(throughput))
				{
					printf("nextDirection: %f %f %f\n", nextDirection.x, nextDirection.y, nextDirection.z);
					printf("f: %f %f %f\n", f.x, f.y, f.z);
					printf("pdf: %f\n", pdf);
				}
				ray = Ray(hitInfo.position, nextDirection);
				ray.position += ray.direction * NXRT_EPSILON;
			}

			if (f.IsZero() || pdf == 0) continue;

			const static float alpha = 0.66666667f; 

			if (bIsIntersect)
			{
				// caustics
				Vector3 pos = hitInfo.position;
				Vector3 norm = hitInfo.shading.normal;

				priority_queue_distance_cartesian<NXPhoton> nearestCausticPhotons([pos](const NXPhoton& photonA, const NXPhoton& photonB) {
					float distA = Vector3::DistanceSquared(pos, photonA.position);
					float distB = Vector3::DistanceSquared(pos, photonB.position);
					return distA < distB;
					});

				UINT causticPhotons;	// 下一次累积的光子数量
				float causticRadius2;
				if (!depth)
				{
					causticPhotons = 50;
					m_pCausticPhotonMap->GetNearest(pos, norm, causticRadius2, nearestCausticPhotons, causticPhotons, 0.15f, LocateFilter::Sphere);
				}
				else
				{
					m_pCausticPhotonMap->GetNearest(pos, norm, causticRadius2, nearestCausticPhotons, -1, pixel.causticRadius2, LocateFilter::Sphere);
					causticPhotons = pixel.causticPhotons + (UINT)(alpha * nearestCausticPhotons.size());
					causticRadius2 = nearestCausticPhotons.empty() ?
						pixel.causticRadius2 :
						pixel.causticRadius2 * ((float)causticPhotons / (pixel.causticPhotons + (UINT)nearestCausticPhotons.size()));
				}

				Vector3 causticFlux(0.0f);
				if (!nearestCausticPhotons.empty())
				{
					while (!nearestCausticPhotons.empty())
					{
						auto photon = nearestCausticPhotons.top();
						Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon.direction, pdfPhotons);
						if (!f.IsZero() && pdfPhotons != 0.0f)
							causticFlux += f * photon.power;
						nearestCausticPhotons.pop();
					}
					causticFlux *= throughput;
				}

				if (depth) causticFlux = (pixel.causticFlux + causticFlux) * causticRadius2 / pixel.causticRadius2;

				LrCaustic = causticFlux / (XM_PI * (float)m_pCausticPhotonMap->GetPhotonCount() * (depth + 1) * causticRadius2);
				pixel.causticFlux = causticFlux;
				pixel.causticPhotons = causticPhotons;
				pixel.causticRadius2 = causticRadius2;

				// multiple diffuse reflections.
				throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
				Ray nextRay = Ray(hitInfo.position, nextDirection);
				nextRay.position += nextRay.direction * NXRT_EPSILON;

				if (Vector3::IsNaN(throughput))
				{
					printf("nextDirection: %f %f %f\n", nextDirection.x, nextDirection.y, nextDirection.z);
					printf("f: %f %f %f\n", f.x, f.y, f.z);
					printf("pdf: %f\n", pdf);
				}

				bIsDiffuse = false;
				NXHit hitInfoDiffuse;
				while (true)
				{
					bIsIntersect = pScene->RayCast(nextRay, hitInfoDiffuse);
					if (!bIsIntersect)
						break;

					hitInfoDiffuse.GenerateBSDF(true);

					std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
					f = hitInfoDiffuse.BSDF->Sample(hitInfoDiffuse.direction, nextDirection, pdf, sampleEvent);
					bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
					sampleEvent.reset();

					if (f.IsZero() || pdf == 0) break;
					if (bIsDiffuse) break;

					throughput *= f * fabsf(hitInfoDiffuse.shading.normal.Dot(nextDirection)) / pdf;
					if (Vector3::IsNaN(throughput))
					{
						printf("nextDirection: %f %f %f\n", nextDirection.x, nextDirection.y, nextDirection.z);
						printf("f: %f %f %f\n", f.x, f.y, f.z);
						printf("pdf: %f\n", pdf);
					}
					nextRay = Ray(hitInfoDiffuse.position, nextDirection);
					nextRay.position += nextRay.direction * NXRT_EPSILON;
				}

				if (f.IsZero() || pdf == 0)
					continue;

				if (bIsIntersect)
				{
					Vector3 posDiff = hitInfoDiffuse.position;
					Vector3 normDiff = hitInfoDiffuse.shading.normal;

					priority_queue_distance_cartesian<NXPhoton> nearestGlobalPhotons([posDiff](const NXPhoton& photonA, const NXPhoton& photonB) {
						float distA = Vector3::DistanceSquared(posDiff, photonA.position);
						float distB = Vector3::DistanceSquared(posDiff, photonB.position);
						return distA < distB;
						});

					UINT globalPhotons;	// 下一次累积的光子数量
					float globalRadius2;
					if (!pixel.estimateFlag)
					{
						globalPhotons = 100;
						m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, globalRadius2, nearestGlobalPhotons, globalPhotons, FLT_MAX, LocateFilter::Sphere);
						pixel.estimateFlag = true;
					}
					else
					{
						m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, globalRadius2, nearestGlobalPhotons, -1, pixel.globalRadius2, LocateFilter::Sphere);
						globalPhotons = pixel.globalPhotons + (UINT)(alpha * nearestGlobalPhotons.size());
						globalRadius2 = nearestGlobalPhotons.empty() ?
							pixel.globalRadius2 :
							pixel.globalRadius2 * ((float)globalPhotons / (pixel.globalPhotons + (UINT)nearestGlobalPhotons.size()));
					}

					if (globalRadius2 == 0.0f)
					{
						printf("globalPhotons: %d\n", globalPhotons);
						printf("pixel.globalRadius2: %f\n", pixel.globalRadius2);
						printf("pixel.globalPhotons: %d\n", pixel.globalPhotons);
						printf("nearestGlobalPhotons.size(): %d\n", (int)nearestGlobalPhotons.size());
					}

					Vector3 globalFlux(0.0f);
					if (!nearestGlobalPhotons.empty())
					{
						while (!nearestGlobalPhotons.empty())
						{
							auto photon = nearestGlobalPhotons.top();
							Vector3 f = hitInfoDiffuse.BSDF->Evaluate(-nextRay.direction, photon.direction, pdfPhotons);
							if (!f.IsZero() && pdfPhotons != 0.0f)
								globalFlux += f * photon.power;
							nearestGlobalPhotons.pop();
						}
						globalFlux *= throughput;
					}

					if (depth) globalFlux = (pixel.globalFlux + globalFlux) * globalRadius2 / pixel.globalRadius2;

					LrGlobal = globalFlux / (XM_PI * (float)m_pGlobalPhotonMap->GetPhotonCount() * (depth + 1) * globalRadius2);
					if (Vector3::IsNaN(LrGlobal))
					{
						printf("throughput: %f %f %f\n", throughput.x, throughput.y, throughput.z);
						printf("globalFlux: %f %f %f\n", globalFlux.x, globalFlux.y, globalFlux.z);
						printf("pixel.globalFlux: %f %f %f\n", pixel.globalFlux.x, pixel.globalFlux.y, pixel.globalFlux.z);
						printf("globalRadius2: %f pixel.globalRadius2: %f", globalRadius2, pixel.globalRadius2);
					}

					pixel.globalFlux = globalFlux;
					pixel.globalPhotons = globalPhotons;
					pixel.globalRadius2 = globalRadius2;
				}

				// final radiance
				pixel.radiance += Le + Ld;

				if (RenderOnce)
				{
					Vector3 result = (pixel.radiance / (float)(depth + 1)) + LrGlobal + LrCaustic;

					if (Vector3::IsNaN(result))
					{
						printf("pixel.radiance: %f %f %f\n", pixel.radiance.x, pixel.radiance.y, pixel.radiance.z);
						printf("LrGlobal: %f %f %f\n", LrGlobal.x, LrGlobal.y, LrGlobal.z);
						printf("LrCaustic: %f %f %f\n", LrCaustic.x, LrCaustic.y, LrCaustic.z);
					}

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
}
