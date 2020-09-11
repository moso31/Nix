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

void NXRayTracer::RenderImage(const std::shared_ptr<NXScene>& pScene, NXRayTraceRenderMode rayTraceMode, bool bCenterRayTest)
{
	XMINT2 renderResolution = XMINT2(800, 600);
	int pixelSample = 4;

	switch (rayTraceMode)
	{
	case NXRayTraceRenderMode::DirectLighting:
	{
		std::make_unique<NXDirectIntegrator>(renderResolution, pixelSample, "D:\\Nix_DirectLighting.bmp")->Render(pScene);
		//std::make_unique<NXDirectIntegrator>(renderResolution, pixelSample, "D:\\Nix_DirectLighting.bmp")->CenterRayTest(pScene);
		break;
	}
	case NXRayTraceRenderMode::PathTracing:
	{
		int pixelSample = 1;
		std::make_unique<NXPathIntegrator>(renderResolution, pixelSample, "D:\\Nix_PathTracing.bmp")->Render(pScene);
		break;
	}
	case NXRayTraceRenderMode::PhotonMapping:
	{
		int nPhotons = 100000;
		int nEstimatePhotons = 500;
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
