#pragma once
#include <execution>
#include "NXRayTracePass.h"
#include "NXDirectIntegrator.h"
#include "ImageGenerator.h"

class NXRayTracePassDirect : public NXRayTracePass
{
public:
	NXRayTracePassDirect() {}
	~NXRayTracePassDirect() {}

	void Load(const std::shared_ptr<NXScene>& pScene, XMINT2 imageSize, XMINT2 tileSize, int eachPixelSamples);
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
	Vector2 m_imageSizeInv;	
	XMINT2 m_tileCount;

	std::shared_ptr<NXDirectIntegrator> m_pIntegrator;
	std::vector<ImageBMPData> pImageData;

	// 进度条
	int m_progress;
};