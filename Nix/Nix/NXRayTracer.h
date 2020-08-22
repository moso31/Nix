#pragma once
#include <atomic>
#include "NXInstance.h"
#include "NXIntegrator.h"
#include "NXIrradianceCache.h"
#include "HBVH.h"
#include "ImageGenerator.h"

// ������Ⱦ��ͼ����Ϣ
struct NXRenderImageInfo
{
	// ͼ��ֱ���
	XMINT2 ImageSize;

	// ÿ�������е���������
	int EachPixelSamples;

	// ���ͼ��·��
	std::string outPath;
};

struct NXRenderTileTaskInfo
{
	XMINT2 tileId;
};

struct NXRenderTileData
{
	XMINT2 tileId;
	std::vector<ImageBMPData> pData;
};

class NXPhotonMap;
class NXRayTracer : public NXInstance<NXRayTracer>
{
public:
	NXRayTracer();
	~NXRayTracer() {}

	void Load(const std::shared_ptr<NXScene>& pScene, const std::shared_ptr<NXCamera>& pMainCamera, const std::shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo);

	void Render();

	// ��������ͼ��
	void MakeImage();

	// ���߳�����ÿ��Tile��
	void MakeImageTile(const int taskIter);

	// Ԥ����IrradianceCache���Զ��������ӹ��ս��м��١�
	std::shared_ptr<NXIrradianceCache> MakeIrradianceCache(const std::shared_ptr<NXPhotonMap>& pGlobalPhotonMap);
	
	// ����Ļ���ķ��򷢳��������ߡ�
	void CenterRayTest(const int testTime = 1);

	// ������Tile���ϳ�һ������ͼ��
	void GenerateImage();

	void Release();

	void Update();

private:
	std::shared_ptr<NXScene> m_pScene;
	std::shared_ptr<NXCamera> m_pRayTraceCamera;
	std::shared_ptr<NXIntegrator> m_pIntegrator;
	std::shared_ptr<NXIrradianceCache> m_pIrradianceCache;
	NXRenderImageInfo m_imageInfo;

	Vector2 m_fImageSizeInv;
	Vector2 m_fNDCToViewSpaceFactorInv;
	Matrix m_mxViewToWorld;
	XMINT2 m_iTileSize;

	std::vector<NXRenderTileTaskInfo> m_renderTileTaskIn;
	std::vector<NXRenderTileData> m_renderTileTaskOut;

	// ����ÿ��renderTileTask��������ȷ����
	// ���Կ���ʹ��һ������������ͷ��β�����������renderTileTask���У������ִ�ж��̡߳�
	std::atomic_int m_iTaskIter;

	std::atomic_int m_iCompletedThreadCount;
	std::atomic_int m_iRunningThreadCount;
	bool m_bIsRayTracing;

	// ��ʱ��
	size_t m_iStartTime;
	size_t m_iEndTime;
};
