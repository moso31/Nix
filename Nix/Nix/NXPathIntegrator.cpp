#include "NXPathIntegrator.h"
#include "NXCubeMap.h"
#include "NXRandom.h"
#include "NXPrimitive.h"

NXPathIntegrator::NXPathIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath) :
	NXSampleIntegrator(imageSize, eachPixelSamples, outPath)
{
}

Vector3 NXPathIntegrator::Radiance(const Ray& ray, const std::shared_ptr<NXScene>& pScene, int depth)
{
	const int maxDepth = 5;
	Vector3 throughput(1.0f);
	Vector3 L(0.0f);
	Ray nextRay(ray);
	bool bIsSpecular(false);
	while (true)
	{
		NXHit hitInfo;
		bool bIsIntersect = pScene->RayCast(nextRay, hitInfo);

		// �ڵ�һ�ε���(depth=0)��ʱ����Ҫ�ֶ�����ֱ�ӹ��ա�
		// ����2-n�ε�����������ص�����ȫ����UniformLightOne�ṩ��
		// ͬ������һ���Ǹ�������(IsSpecular)�������ҲҪ�ֶ�����ȫ�ֹ��ա�
		// ��Ϊ����delta�ֲ��������ԣ����������UniformLightOne�ṩ�����Ŀ�������0��
		if (depth == 0 || bIsSpecular)
		{
			if (!bIsIntersect)
			{
				auto pCubeMap = pScene->GetCubeMap();
				auto pCubeMapLight = pCubeMap->GetEnvironmentLight();
				if (!pCubeMap || !pCubeMapLight)
					break;

				L += throughput * pCubeMapLight->GetRadiance(Vector3(), Vector3(), -nextRay.direction);
				break;
			}

			std::shared_ptr<NXPBRTangibleLight> pTangibleLight = hitInfo.pPrimitive->GetTangibleLight();
			if (pTangibleLight)
			{
				L += throughput * pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
			}
		}

		if (!bIsIntersect) break;

		hitInfo.GenerateBSDF(true);
		L += throughput * UniformLightOne(nextRay, pScene, hitInfo);
		
		// depth����ָ����������ڴ˴���������Կ������ε�������֮��Ľ��硣
		// ������throughput�ļ��㣬ʵ��������Ϊ��һ�������׼����
		if (depth++ > maxDepth) break;

		float pdf;
		Vector3 nextDirection;
		std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
		Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsSpecular = *sampleEvent & NXBSDF::DELTA;
		sampleEvent.reset();

		if (f.IsZero()) break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		nextRay = Ray(hitInfo.position, nextDirection);
		nextRay.position += nextRay.direction * NXRT_EPSILON;

		if (depth > 3)
		{
			// q�����ֵ��Ϊ0.05��������20��������������1������Ч������
			// �����ڵ�Ƶ�����ĵط�ѡ���ʹ��͡�
			float q = max(0.05f, throughput.MaxComponent());
			float fRandom = NXRandom::GetInstance()->CreateFloat();
			if (fRandom < 1.0f - q)
				break;
			throughput /= q;
		}
	}

	return L;
}
