#pragma once
#include <atomic>
#include "NXInstance.h"
#include "NXIntegrator.h"
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

class NXRayTracer : public NXInstance<NXRayTracer>
{
public:
	NXRayTracer();
	~NXRayTracer() {}

	// ��������ͼ��
	void MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo);

	// ���߳�����ÿ��Tile��
	void MakeImageTile(const int taskIter);
	
	// ����Ļ���ķ��򷢳��������ߡ�
	void CenterRayTest(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const int testTime = 1);

	// ������Tile���ϳ�һ������ͼ��
	void GenerateImage();

	void Release();

	void Update();

private:
	shared_ptr<NXScene> m_pScene;
	shared_ptr<NXIntegrator> m_pIntegrator;
	NXRenderImageInfo m_imageInfo;

	Vector2 m_fImageSizeInv;
	Vector2 m_fNDCToViewSpaceFactorInv;
	Matrix m_mxViewToWorld;
	XMINT2 m_iTileSize;

	vector<NXRenderTileTaskInfo> m_renderTileTaskIn;
	vector<NXRenderTileData> m_renderTileTaskOut;

	// ����ÿ��renderTileTask��������ȷ����
	// ���Կ���ʹ��һ������������ͷ��β�����������renderTileTask���У������ִ�ж��̡߳�
	atomic_int m_iTaskIter;

	atomic_int m_iCompletedThreadCount;
	atomic_int m_iRunningThreadCount;
	bool m_bIsRayTracing;

	// ��ʱ��
	size_t m_iStartTime;
	size_t m_iEndTime;
};
