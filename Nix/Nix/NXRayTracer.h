#pragma once
#include "header.h"

class NXIntegrator;

// 离线渲染的图像信息
struct NXRenderImageInfo
{
	// 图像分辨率
	XMINT2 ImageSize;

	// 每个像素中的样本数量
	int SamplerEachPixel;
};

class NXRayTracer
{
public:
	NXRayTracer() {};
	~NXRayTracer() {};

	void MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo);

	void Release();

private:
	
};
