#pragma once
#include "header.h"

class NXIntegrator;

class BSDF
{
public:
	BSDF() {}
	~BSDF() {}

	void ();

private:
	Vector3 p;
	Vector3 wi;
};

// 离线渲染的图像信息
struct NXRenderImageInfo
{
	// 图像分辨率
	XMINT2 ImageSize;

	// 每个像素中的样本数量
	int EachPixelSamples;
};

class NXRayTracer
{
public:
	NXRayTracer();
	~NXRayTracer() {}

	void MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo);
	void RayCast(const shared_ptr<NXScene>& pScene, const Ray& rayWorld, const shared_ptr<NXIntegrator>& pIntegrator);

	void Release();

private:
};
