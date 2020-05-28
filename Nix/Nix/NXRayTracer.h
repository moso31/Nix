#pragma once
#include "NXInstance.h"
#include "NXIntegrator.h"
#include "HBVH.h"

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
