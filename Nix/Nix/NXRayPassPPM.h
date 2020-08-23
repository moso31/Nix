#pragma once
#include <execution>
#include "ImageGenerator.h"
#include "NXRayTracePass.h"
#include "NXPPMPixel.h"
#include "NXPhoton.h"

class NXRayPassPPMGeneratePixels : public NXRayTracePass
{
public:
	NXRayPassPPMGeneratePixels() {}
	~NXRayPassPPMGeneratePixels() {}

	void Load(const std::shared_ptr<NXScene>& pScene, const XMINT2& imageSize, const XMINT2& tileSize, int eachPixelSamples);
	void Render() override;

	std::shared_ptr<NXPPMPixelGenerator> GetPPMPixelGenerator() { return m_pPPMPixelGenerator; }

private:
	void RenderImageDataParallel(bool useOpenMP);
	void RenderTile(const XMINT2& tileId);

private:
	std::shared_ptr<NXScene> m_pScene;
	std::shared_ptr<NXPPMPixelGenerator> m_pPPMPixelGenerator;

	XMINT2 m_imageSize;			// 当前Pass的分辨率大小
	XMINT2 m_tileSize;			// 如果需要分tile多线程计算
	XMINT2 m_tileCount;			// tile数量
	int m_eachPixelSamples;		// 每个像素多少样本 

	std::mutex m_mutexPixelInfo;	// 在使用多线程添加PixelInfo数据时需要使用互斥锁。
};

class NXRayPassPPM : public NXRayTracePass
{
public:
	NXRayPassPPM() {}
	~NXRayPassPPM() {}

	void Load(const std::shared_ptr<NXScene>& pScene, const std::shared_ptr<NXPPMPixelGenerator>& pPPMPixelGenerator, const XMINT2& imageSize, int nPhotonsAtOnce);
	void GeneratePhotonMap();
	void Render() override;

	void Release();

private:
	void RenderOneSample(ImageBMPData* pImageData, PPMPixel& pixel);

private:
	std::shared_ptr<NXScene> m_pScene;
	std::shared_ptr<NXPPMPixelGenerator> m_pPPMPixelGenerator;
	std::shared_ptr<NXPhotonMap> m_pPhotonMap;
	ImageBMPData* m_pImageData;	// 最终图像的数据

	XMINT2 m_imageSize;			// 当前Pass的分辨率大小

	int m_accumulatePhotons;	// 累积光子总数
	int m_nPhotonsAtOnce;	// 一次pass迭代所生成的光子数量
};