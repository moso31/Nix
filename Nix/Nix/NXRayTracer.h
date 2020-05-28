#pragma once
#include "NXInstance.h"
#include "NXIntegrator.h"
#include "HBVH.h"

// 离线渲染的图像信息
struct NXRenderImageInfo
{
	// 图像分辨率
	XMINT2 ImageSize;

	// 每个像素中的样本数量
	int EachPixelSamples;

	// 输出图像路径
	string outPath;
};

struct ImageBMPData;
class NXRayTracer : public NXInstance<NXRayTracer>
{
public:
	NXRayTracer();
	~NXRayTracer() {}

	void MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo);
	void MakeImageTile(const shared_ptr<NXScene>& pScene, const shared_ptr<NXIntegrator>& pIntegrator, const Matrix& mxViewToWorld, const NXRenderImageInfo& ImageInfo, const Vector2& imageSizeInv, const Vector2& NDCToViewSpaceFactorInv, const XMINT2& tileSize, const XMINT2& tileId, ImageBMPData* pRGB);
	void CenterRayTest(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator);

	void Release();
};
