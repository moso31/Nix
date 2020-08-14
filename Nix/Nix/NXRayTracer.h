#pragma once
#include <atomic>
#include "NXInstance.h"
#include "NXIntegrator.h"
#include "NXIrradianceCache.h"
#include "HBVH.h"
#include "ImageGenerator.h"

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

struct NXRenderTileTaskInfo
{
	XMINT2 tileId;
};

struct NXRenderTileData
{
	XMINT2 tileId;
	vector<ImageBMPData> pData;
};

class NXPhotonMap;
class NXRayTracer : public NXInstance<NXRayTracer>
{
public:
	NXRayTracer();
	~NXRayTracer() {}

	void Load(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo);

	// 生成整个图像。
	void MakeImage();

	// 多线程生成每个Tile。
	void MakeImageTile(const int taskIter);

	// 预计算IrradianceCache，以对漫反射间接光照进行加速。
	void MakeIrradianceCache(const shared_ptr<NXPhotonMap>& pGlobalPhotonMap);
	
	// 向屏幕中心方向发出测试射线。
	void CenterRayTest(const int testTime = 1);

	// 将所有Tile整合成一张最终图像。
	void GenerateImage();

	void Release();

	void Update();

private:
	shared_ptr<NXScene> m_pScene;
	shared_ptr<NXCamera> m_pRayTraceCamera;
	shared_ptr<NXIntegrator> m_pIntegrator;
	shared_ptr<NXIrradianceCache> m_pIrradianceCache;
	NXRenderImageInfo m_imageInfo;

	Vector2 m_fImageSizeInv;
	Vector2 m_fNDCToViewSpaceFactorInv;
	Matrix m_mxViewToWorld;
	XMINT2 m_iTileSize;

	vector<NXRenderTileTaskInfo> m_renderTileTaskIn;
	vector<NXRenderTileData> m_renderTileTaskOut;

	// 由于每次renderTileTask的总量是确定的
	// 所以可以使用一个迭代计数从头到尾按序遍历整个renderTileTask队列，并逐个执行多线程。
	atomic_int m_iTaskIter;

	atomic_int m_iCompletedThreadCount;
	atomic_int m_iRunningThreadCount;
	bool m_bIsRayTracing;

	// 计时器
	size_t m_iStartTime;
	size_t m_iEndTime;
};
