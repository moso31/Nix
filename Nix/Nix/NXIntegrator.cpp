#include "NXIntegrator.h"
#include "NXBSDF.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "NXPBRLight.h"
#include "NXRandom.h"
#include "NXRayTracer.h"
#include "SamplerMath.h"

using namespace SamplerMath;

Vector3 NXIntegrator::DirectEstimate(const Ray& ray, const std::shared_ptr<NXScene>& pScene, const std::shared_ptr<NXPBRLight>& pLight, const NXHit& hitInfo)
{
	Vector3 L(0.0f);

	// 统计pdf时，不统计带有SPECULAR类型的反射模型。
	bool bIsDeltaLight = pLight->IsDeltaLight();

	if (bIsDeltaLight)
	{
		Vector3 incidentDirection;
		float pdfLight = 0.0f;
		Vector3 Li = pLight->Illuminate(hitInfo, incidentDirection, pdfLight);
		if (!Li.IsZero())
		{
			float pdfBSDF;
			Vector3 f = hitInfo.BSDF->Evaluate(hitInfo.direction, incidentDirection, pdfBSDF);
			if (!f.IsZero())
			{
				L = f * Li * incidentDirection.Dot(hitInfo.shading.normal);
			}
		}
	}
	else
	{
		Vector3 f(0.0f);
		Vector3 incidentDirection;
		float pdfLight = 0.0f, pdfBSDF = 0.0f, pdfWeight = 0.0f;

		// 基于光源采样一次
		Vector3 Li = pLight->Illuminate(hitInfo, incidentDirection, pdfLight);
		if (!Li.IsZero() && pdfLight != 0)
		{
			f = hitInfo.BSDF->Evaluate(hitInfo.direction, incidentDirection, pdfBSDF);
			if (!f.IsZero())
			{
				// 这里计算出pdfWeight，但先不用。等后面代码确认了并非DeltaBSDF后再使用。
				pdfWeight = PowerHeuristicWeightPdf(1, pdfLight, 1, pdfBSDF);
				L += f * Li * incidentDirection.Dot(hitInfo.shading.normal) / pdfLight;
			}
		}

		// 基于BSDF采样一次
		std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
		f = hitInfo.BSDF->Sample(hitInfo.direction, incidentDirection, pdfBSDF, sampleEvent);
		bool bIsDeltaBSDF = *sampleEvent & NXBSDF::DELTA;

		// 如果是DeltaBSDF，不使用重点采样，仅使用灯光采样。
		// 否则会被重复迭代。
		if (bIsDeltaBSDF)
			return L;

		// 确认了并非DeltaBSDF后补上上面计算中的pdfWeight。
		L *= pdfWeight;

		if (!f.IsZero() && pdfBSDF != 0)
		{
			// 基于BSDF采样的方向寻找此次采样是否击中光源。
			// 击中光源：使用该光源的自发光数据作为BSDF样本的Li。
			// 未击中光源：使用环境贴图的自发光作为Li（没有环境贴图则返回0.）
			Ray ray(hitInfo.position, incidentDirection);
			ray.position += incidentDirection * NXRT_EPSILON;
			Vector3 Li(0.0f);
			NXHit hitLightInfo;
			pScene->RayCast(ray, hitLightInfo);
			std::shared_ptr<NXPBRAreaLight> pHitAreaLight;
			if (hitLightInfo.pPrimitive)
			{
				pHitAreaLight = hitLightInfo.pPrimitive->GetTangibleLight();
			}
			else if (pScene->GetCubeMap())
			{
				pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();
			}

			if (pHitAreaLight == pLight)
			{
				Li = pHitAreaLight->GetRadiance(hitLightInfo.position, hitLightInfo.normal, incidentDirection);
				pdfLight = pHitAreaLight->GetPdf(hitInfo, hitLightInfo.position, hitLightInfo.normal, hitLightInfo.direction);

				if (!Li.IsZero())
				{
					// 计算权重。对基于BSDF的采样，BSDF为主要加权，Light其次。
					pdfWeight = PowerHeuristicWeightPdf(1, pdfBSDF, 1, pdfLight);
					L += f * Li * incidentDirection.Dot(hitInfo.shading.normal) * pdfWeight / pdfBSDF;
				}
			}
		}
	}

	return L;
}

Vector3 NXIntegrator::UniformLightAll(const Ray& ray, const std::shared_ptr<NXScene>& pScene, const NXHit& hitInfo)
{
	// All: 统计所有的光源
	Vector3 result(0.0f);
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
		result += DirectEstimate(ray, pScene, *it, hitInfo);
	return result;
}

Vector3 NXIntegrator::UniformLightOne(const Ray& ray, const std::shared_ptr<NXScene>& pScene, const NXHit& hitInfo)
{
	// One: 仅统计单个光源，但对光源的选取完全随机。此方法期望值和All方法等同。
	Vector3 result(0.0f);
	auto pLights = pScene->GetPBRLights();
	int lightCount = (int)pLights.size();
	int index = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);
	return DirectEstimate(ray, pScene, pLights[index], hitInfo) * (float)lightCount;
}

NXSampleIntegrator::NXSampleIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath) :
	NXIntegrator(imageSize),
	m_eachPixelSamples(eachPixelSamples),
	m_outFilePath(outPath),
	m_tileSize(XMINT2(64, 64))
{
}

void NXSampleIntegrator::Render(const std::shared_ptr<NXScene>& pScene)
{
	printf("Building BVH Trees...");
	pScene->BuildBVHTrees(HLBVH);
	printf("Done.\n");

	printf("Rendering...");
	int pixelCount = m_imageSize.x * m_imageSize.y;
	ImageBMPData* pImageData = new ImageBMPData[pixelCount];
	memset(pImageData, 0, sizeof(ImageBMPData) * pixelCount);

	m_progress = 0;
	XMINT2 tileCount = XMINT2((m_imageSize.x + m_tileSize.x - 1) / m_tileSize.x, (m_imageSize.y + m_tileSize.y - 1) / m_tileSize.y);
	int nTiles = tileCount.x * tileCount.y;

	bool useOpenMP = false;
	// 两种并行方案：OpenMP 或 C++17 execution
	if (useOpenMP)
	{
#pragma omp parallel for
		for (int tx = 0; tx < tileCount.x; tx++)
			for (int ty = 0; ty < tileCount.y; ty++)
			{
				RenderTile(pScene, XMINT2(tx, ty), pImageData);
			}
	}
	else
	{
		std::vector<XMINT2> tasks;
		for (int tx = 0; tx < tileCount.x; tx++)
			for (int ty = 0; ty < tileCount.y; ty++)
				tasks.push_back(XMINT2(tx, ty));

		std::for_each(std::execution::par, tasks.begin(), tasks.end(), [this, pScene, pImageData, nTiles](const XMINT2& tileId) {
			RenderTile(pScene, tileId, pImageData);
			printf("\r%.2f%% ", (float)++m_progress * 100.0f / (float)nTiles);	// 进度条更新
			});
	}

	ImageGenerator::GenerateImageBMP((byte*)pImageData, m_imageSize.x, m_imageSize.y, m_outFilePath.c_str());
	delete pImageData;
	printf("done.\n");
}

void NXSampleIntegrator::RenderTile(const std::shared_ptr<NXScene>& pScene, const XMINT2& tileId, ImageBMPData* oImageData)
{
	for (int i = 0; i < m_tileSize.x; i++)
	{
		for (int j = 0; j < m_tileSize.y; j++)
		{
			Vector3 result(0.0f);
			int pixelX = tileId.x * m_tileSize.x + i;
			int pixelY = tileId.y * m_tileSize.y + j;
			if (pixelX >= m_imageSize.x || pixelY >= m_imageSize.y)
				continue;

			Vector2 pixelCoord((float)pixelX, (float)pixelY);
			for (UINT pixelSample = 0; pixelSample < m_eachPixelSamples; pixelSample++)
			{
				// pixel + [0, 1)^2.
				Vector2 sampleCoord = pixelCoord + NXRandom::GetInstance()->CreateVector2();

				Ray rayWorld = pScene->GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));
				result += Radiance(rayWorld, pScene, 0);
			}
			result /= (float)m_eachPixelSamples;

			XMINT3 RGBValue(
				result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
				result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
				result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

			int index = (m_tileSize.y - j - 1) * m_tileSize.x + i;
			int rgbIdx = (m_imageSize.y - pixelY - 1) * m_imageSize.x + pixelX;
			oImageData[rgbIdx].r = RGBValue.x;
			oImageData[rgbIdx].g = RGBValue.y;
			oImageData[rgbIdx].b = RGBValue.z;
		}
	}
}

Vector3 NXSampleIntegrator::CenterRayTest(const std::shared_ptr<NXScene>& pScene)
{
	Vector2 sampleCoord = Vector2((float)m_imageSize.x * 0.5f, (float)m_imageSize.y * 0.5f);
	Ray rayWorld = pScene->GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));
	Vector3 result = Radiance(rayWorld, pScene, 0);
	printf("center ray test: %f %f %f\n", result.x, result.y, result.z);
	return result;
}
