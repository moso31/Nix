#pragma once
#include <execution>
#include "ImageGenerator.h"
#include "NXRayTracePass.h"
#include "NXPMSplitIntegrator.h"
#include "NXIrradianceCache.h"

class NXRayTracePassPMSplitPhotonMap : public NXRayTracePass
{
public:
	NXRayTracePassPMSplitPhotonMap() {}
	~NXRayTracePassPMSplitPhotonMap() {}

	void Load(const std::shared_ptr<NXScene>& pScene, int numCausticPhotons, int numGlobalPhotons);
	void Render() override;

	std::shared_ptr<NXPhotonMap> GetCausticPhotonMap() { return m_pCausticPhotonMap; }
	std::shared_ptr<NXPhotonMap> GetGlobalPhotonMap() { return m_pGlobalPhotonMap; }

private:
	std::shared_ptr<NXScene> m_pScene;
	std::shared_ptr<NXPhotonMap> m_pGlobalPhotonMap;
	std::shared_ptr<NXPhotonMap> m_pCausticPhotonMap;
	int m_numGlobalPhotons;
	int m_numCausticPhotons;
};

class NXRayTracePassPMSplitIrradianceCache : public NXRayTracePass
{
public:
	NXRayTracePassPMSplitIrradianceCache() {}
	~NXRayTracePassPMSplitIrradianceCache() {}

	void Load(const std::shared_ptr<NXScene>& pScene, const XMINT2& imageSize, const std::shared_ptr<NXPhotonMap>& pPhotonMap);
	void Render() override;

	std::shared_ptr<NXIrradianceCache> GetIrradianceCache() { return m_pIrradianceCache; }

private:
	std::shared_ptr<NXScene> m_pScene;
	XMINT2 m_imageSize;

	std::shared_ptr<NXIrradianceCache> m_pIrradianceCache;
};

class NXRayTracePassPMSplit : public NXRayTracePass
{
public:
	NXRayTracePassPMSplit() {}
	~NXRayTracePassPMSplit() {}

	void Load(const std::shared_ptr<NXScene>& pScene, const std::shared_ptr<NXPhotonMap>& pGlobalPhotons, const std::shared_ptr<NXPhotonMap>& pCausticPhotons, const std::shared_ptr<NXIrradianceCache>& pIrradianceCache, const XMINT2& imageSize, const XMINT2& tileSize, int eachPixelSamples);
	void Render() override;

private:
	void RenderImageDataParallel(ImageBMPData* pImageData, bool useOpenMP);
	void RenderTile(ImageBMPData* pImageData, const XMINT2& tileId);

private:
	std::shared_ptr<NXScene> m_pScene;

	XMINT2 m_imageSize;			// 当前Pass的分辨率大小
	XMINT2 m_tileSize;			// 如果需要分tile多线程计算
	int m_eachPixelSamples;		// 每个像素多少样本 

	// 加速计算
	XMINT2 m_tileCount;

	std::shared_ptr<NXPMSplitIntegrator> m_pIntegrator;
	std::vector<ImageBMPData> pImageData;

	// 进度条
	int m_progress;
};