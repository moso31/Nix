#pragma once
#include "NXIntegrator.h"
#include "NXPhoton.h"
#include "NXBSDF.h"

struct NXSPPMPixel
{
	NXSPPMPixel() : 
		causticFlux(0.0f), causticPhotons(0), causticRadius2(0.0f), 
		globalFlux(0.0f), globalPhotons(0), globalRadius2(0.0f), 
		radiance(0.0f), causticEstimateFlag(false), globalEstimateFlag(false) {}
	~NXSPPMPixel() {}

	Vector3 causticFlux, globalFlux;
	UINT causticPhotons, globalPhotons;
	float causticRadius2, globalRadius2;
	Vector3 radiance;
	bool causticEstimateFlag, globalEstimateFlag;
};

class NXSPPMIntegrator : public NXIntegrator
{
public:
	NXSPPMIntegrator(const XMINT2& imageSize, std::string outPath, UINT nCausticPhotons, UINT nGlobalPhotons);
	~NXSPPMIntegrator() {}

	void Render(NXScene* pScene);
	void RefreshPhotonMap(NXScene* pScene);

private:
	void RenderWithPM(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int depth, bool RenderOnce);
	void RenderWithPMSplit(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int depth, bool RenderOnce);

	// 渲染指定区域
	void RenderWithPMArea(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int depth, bool RenderOnce, int x, int y, int offsetX, int offsetY);
	void RenderWithPMSplitArea(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int depth, bool RenderOnce, int x, int y, int offsetX, int offsetY);

	// 渲染单个像素 
	void RenderWithPM(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int depth, bool RenderOnce, int x, int y);
	void RenderWithPMSplit(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int depth, bool RenderOnce, int x, int y);



private:
	NXPhotonMap* m_pCausticPhotonMap;
	NXPhotonMap* m_pGlobalPhotonMap;
	std::string m_outFilePath;

	// 每次步进所需的光子数量
	UINT m_numGlobalPhotonsEachStep;
	UINT m_numCausticPhotonsEachStep;
};

