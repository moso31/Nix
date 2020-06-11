#include "NXPathIntegrator.h"
#include "NXCubeMap.h"
#include "NXRandom.h"

Vector3 NXPathIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
{
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

			shared_ptr<NXTangibleLight> pTangibleLight = hitInfo.pPrimitive->GetTangibleLight();
			if (pTangibleLight)
			{
				L += throughput * pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
			}
		}

		if (!bIsIntersect) break;

		hitInfo.ConstructReflectionModel();
		L += throughput * UniformLightOne(nextRay, pScene, hitInfo);
		
		// depth����ָ����������ڴ˴���������Կ������ε�������֮��Ľ��硣
		// ������throughput�ļ��㣬ʵ��������Ϊ��һ�������׼����
		if (depth++ > 5) break;	

		float pdf;
		Vector3 nextDirection;
		shared_ptr<ReflectionType> outReflectType = make_shared<ReflectionType>();
		Vector3 f = hitInfo.BSDF->Sample_f(hitInfo.direction, nextDirection, pdf, REFLECTIONTYPE_ALL, outReflectType);
		bIsSpecular = (*outReflectType & REFLECTIONTYPE_SPECULAR);
		outReflectType.reset();

		if (f.IsZero()) break;

		throughput *= f * fabsf(hitInfo.normal.Dot(nextDirection)) / pdf;
		nextRay = Ray(hitInfo.position, nextDirection);
		nextRay.position += nextRay.direction * NXRT_EPSILON;
	}

	return L;
}
