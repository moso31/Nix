#include "NXRayTracer.h"
#include "NXScene.h"
#include "NXDirectIntegrator.h"
#include "NXPathIntegrator.h"
#include "NXPMIntegrator.h"
#include "NXPMSplitIntegrator.h"
#include "NXSPPMIntegrator.h"

NXRayTracer::NXRayTracer()
{
}

void NXRayTracer::RenderImage(const std::shared_ptr<NXScene>& pScene, NXRayTraceRenderMode rayTraceMode)
{
	XMINT2 renderResolution = XMINT2(800, 600);
	int pixelSample = 4;

	switch (rayTraceMode)
	{
	case NXRayTraceRenderMode::DirectLighting:
	{
		std::make_unique<NXDirectIntegrator>(renderResolution, pixelSample, "D:\\Nix_DirectLighting.bmp")->Render(pScene);
		break;
	}
	case NXRayTraceRenderMode::PathTracing:
	{
		std::make_unique<NXPathIntegrator>(renderResolution, pixelSample, "D:\\Nix_PathTracing.bmp")->Render(pScene);
		break;
	}
	case NXRayTraceRenderMode::PhotonMapping:
	{
		int nPhotons = 100000;
		int nEstimatePhotons = 100;
		std::make_unique<NXPMIntegrator>(renderResolution, pixelSample, "D:\\Nix_PhotonMapping.bmp", nPhotons, nEstimatePhotons)->Render(pScene);
		break;
	}
	case NXRayTraceRenderMode::IrradianceCache:
	{
		int nCausticPhotons = 50000;
		int nGlobalPhotons = 200000;
		std::make_unique<NXPMSplitIntegrator>(renderResolution, pixelSample, "D:\\Nix_PhotonMappingSplit.bmp", nCausticPhotons, nGlobalPhotons)->Render(pScene);
		break;
	}
	case NXRayTraceRenderMode::SPPM:
	{
		int nPhotons = 200000;
		std::make_unique<NXSPPMIntegrator>(renderResolution,  "D:\\Nix_SPPM.bmp", nPhotons)->Render(pScene);
		break;
	}
	default:
		break;
	}
}

//void NXRayTracer::CenterRayTest(const int testTime)
//{
//	Matrix mxViewToWorld = m_pRayTraceCamera->GetViewMatrix().Invert();
//
//	Vector3 viewDir(0.0f, 0.0f, 1.0f);
//	Ray rayView(Vector3(0.0f), viewDir);
//	Ray rayWorld = rayView.Transform(mxViewToWorld);	// 获取该射线的world空间坐标值
//
//	Vector3 result(0.0f);
//	for (int i = 0; i < testTime; i++)
//	{
//		//Vector3 L = m_pIntegrator->Radiance(rayWorld, m_pScene, 0);
//		//printf("%d: %f, %f, %f\n", i, L.x, L.y, L.z);
//		//result += L;
//	}
//	result /= (float)testTime;
//
//	printf("%f, %f, %f\n", result.x, result.y, result.z);
//}
