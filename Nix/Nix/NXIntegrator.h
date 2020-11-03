#pragma once
#include <execution>
#include "NXIntersection.h"
#include "NXScene.h"
#include "ImageGenerator.h"

class NXIntegrator
{
public:
	NXIntegrator() {}
	NXIntegrator(const XMINT2& imageSize) : m_imageSize(imageSize) {}
	~NXIntegrator() {}

	/*
	对单次采样进行评估。
	如果是DeltaLight：
		仅对light提供的方向进行计算即可得到实际精确值。
	如果不是DeltaLight：
		无法得到精确值，就使用两种方法各采样一次：
			1.基于light采样一次，
			2.基于BSDF采样一次。
		之后将两次采样的结果结合，得到一个较为准确的估算值。
	*/
	Vector3 DirectEstimate(const Ray& ray, NXScene* pScene, NXPBRLight* pLight, const NXHit& hitInfo);

	Vector3 UniformLightAll(const Ray& ray, NXScene* pScene, const NXHit& hitInfo);
	Vector3 UniformLightOne(const Ray& ray, NXScene* pScene, const NXHit& hitInfo);

	virtual void Render(NXScene* pScene) = 0;

protected:
	XMINT2 m_imageSize;		// 渲染图像分辨率

	// 进度条
	UINT m_progress;
};

// 离线渲染的图像信息
struct NXRenderImageInfo
{
	// 图像分辨率
	XMINT2 ImageSize;

	// 每个像素中的样本数量
	int EachPixelSamples;

	// 输出图像路径
	std::string outPath;
};

class NXSampleIntegrator : public NXIntegrator
{
public:
	NXSampleIntegrator() {}
	NXSampleIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath);
	~NXSampleIntegrator() {}

	void Render(NXScene* pScene) override;
	virtual Vector3 Radiance(const Ray& ray, NXScene* pScene, int depth) = 0;
	Vector3 CenterRayTest(NXScene* pScene);

private:
	void RenderTile(NXScene* pScene, const XMINT2& tileId, ImageBMPData* oImageData);

public:
	// 渲染图片存放路径
	std::string m_outFilePath;

private:
	XMINT2 m_tileSize;		// 单个并行渲染Tile的大小
	UINT m_eachPixelSamples;	// 单个像素样本数量
};