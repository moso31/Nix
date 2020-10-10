#include "NXRayTracer.h"
#include "NXScene.h"
#include "NXDirectIntegrator.h"
#include "NXPathIntegrator.h"
#include "NXPMIntegrator.h"
#include "NXPMSplitIntegrator.h"
#include "NXSPPMIntegrator.h"
#include "NXTimer.h"

NXRayTracer::NXRayTracer()
{
}

void NXRayTracer::RenderImage(const std::shared_ptr<NXScene>& pScene, NXRayTraceRenderMode rayTraceMode, bool bCenterRayTest)
{
	NXTimer t1, t2;
	t1.Tick();

	XMINT2 renderResolution = XMINT2(800, 600);
	int pixelSample = 16;

	switch (rayTraceMode)
	{
	case NXRayTraceRenderMode::DirectLighting:
	{
		printf("DirectLighting Integrator Running...\n");
		//std::make_unique<NXDirectIntegrator>(renderResolution, pixelSample, "D:\\Nix_DirectLighting.bmp")->Render(pScene);
		std::make_unique<NXDirectIntegrator>(renderResolution, pixelSample, "D:\\Nix_DirectLighting.bmp")->CenterRayTest(pScene);
		break;
	}
	case NXRayTraceRenderMode::PathTracing:
	{
		printf("PathTracing Integrator Running...\n");
		int pixelSample = 64;
		std::make_unique<NXPathIntegrator>(renderResolution, pixelSample, "D:\\Nix_PathTracing.bmp")->Render(pScene);
		//std::make_unique<NXPathIntegrator>(renderResolution, pixelSample, "D:\\Nix_PathTracing.bmp")->CenterRayTest(pScene);
		break;
	}
	case NXRayTraceRenderMode::PhotonMapping:
	{
		printf("PhotonMapping Integrator Running...\n");
		int nPhotons = 100000;
		int nEstimatePhotons = 500;
		std::make_unique<NXPMIntegrator>(renderResolution, pixelSample, "D:\\Nix_PhotonMapping.bmp", nPhotons, nEstimatePhotons)->Render(pScene);
		break;
	}
	case NXRayTraceRenderMode::IrradianceCache:
	{
		printf("IrradianceCache Integrator Running...\n");
		int nCausticPhotons = 50000;
		int nGlobalPhotons = 200000;
		std::make_unique<NXPMSplitIntegrator>(renderResolution, pixelSample, "D:\\Nix_PhotonMappingSplit.bmp", nCausticPhotons, nGlobalPhotons)->Render(pScene);
		break;
	}
	case NXRayTraceRenderMode::SPPM:
	{
		printf("SPPM Integrator Running...\n");
		int nCausticPhotons = 50000;
		int nGlobalPhotons = 200000;
		std::make_unique<NXSPPMIntegrator>(renderResolution,  "D:\\Nix_SPPM.bmp", nCausticPhotons, nGlobalPhotons)->Render(pScene);
		break;
	}
	default:
		break;
	}
	t2.Tick();

	float timeCost = (float)(t2.GetTimeDelta() - t1.GetTimeDelta());
	printf("%f\n", timeCost * 1e-6f);
}
