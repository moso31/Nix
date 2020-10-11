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

	// ͳ��pdfʱ����ͳ�ƴ���SPECULAR���͵ķ���ģ�͡�
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

		// ���ڹ�Դ����һ��
		Vector3 Li = pLight->Illuminate(hitInfo, incidentDirection, pdfLight);
		if (!Li.IsZero() && pdfLight != 0)
		{
			f = hitInfo.BSDF->Evaluate(hitInfo.direction, incidentDirection, pdfBSDF);
			if (!f.IsZero())
			{
				// ��������pdfWeight�����Ȳ��á��Ⱥ������ȷ���˲���DeltaBSDF����ʹ�á�
				pdfWeight = PowerHeuristicWeightPdf(1, pdfLight, 1, pdfBSDF);
				L += f * Li * incidentDirection.Dot(hitInfo.shading.normal) / pdfLight;
			}
		}

		// ����BSDF����һ��
		std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
		f = hitInfo.BSDF->Sample(hitInfo.direction, incidentDirection, pdfBSDF, sampleEvent);
		bool bIsDeltaBSDF = *sampleEvent & NXBSDF::DELTA;

		// �����DeltaBSDF����ʹ���ص��������ʹ�õƹ������
		// ����ᱻ�ظ�������
		if (bIsDeltaBSDF)
			return L;

		// ȷ���˲���DeltaBSDF������������е�pdfWeight��
		L *= pdfWeight;

		if (!f.IsZero() && pdfBSDF != 0)
		{
			// ����BSDF�����ķ���Ѱ�Ҵ˴β����Ƿ���й�Դ��
			// ���й�Դ��ʹ�øù�Դ���Է���������ΪBSDF������Li��
			// δ���й�Դ��ʹ�û�����ͼ���Է�����ΪLi��û�л�����ͼ�򷵻�0.��
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
					// ����Ȩ�ء��Ի���BSDF�Ĳ�����BSDFΪ��Ҫ��Ȩ��Light��Ρ�
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
	// All: ͳ�����еĹ�Դ
	Vector3 result(0.0f);
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
		result += DirectEstimate(ray, pScene, *it, hitInfo);
	return result;
}

Vector3 NXIntegrator::UniformLightOne(const Ray& ray, const std::shared_ptr<NXScene>& pScene, const NXHit& hitInfo)
{
	// One: ��ͳ�Ƶ�����Դ�����Թ�Դ��ѡȡ��ȫ������˷�������ֵ��All������ͬ��
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
	// ���ֲ��з�����OpenMP �� C++17 execution
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
			printf("\r%.2f%% ", (float)++m_progress * 100.0f / (float)nTiles);	// ����������
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
