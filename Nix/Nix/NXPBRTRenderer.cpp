#include "NXPBRTRenderer.h"
#include <SamplerMath.h>

#include "NXScene.h"
#include "NXIntersection.h"
#include "NXPhotonMap.h"

using namespace SamplerMath;

NXPBRTRenderer::NXPBRTRenderer()
{
	int seed = 0;	// can be changed as time.
	m_rng = default_random_engine(seed);
}

void NXPBRTRenderer::GeneratePhotonMap(const shared_ptr<NXScene>& pScene, const NXPhotonMap& photonMapInfo)
{
	auto pLights = pScene->GetPBRLights();
	float fEachLightPhotonsInv = (float)pLights.size() / (float)photonMapInfo.Photons;

	// 随机光源发射随机光子
	int iCount = 0;
	while(true)
	{
		if (iCount >= photonMapInfo.Photons)
			return;

		uniform_int_distribution<int> iRandom(0, (int)pLights.size() - 1);
		shared_ptr<NXPBRPointLight> pLight = pLights[iRandom(m_rng)];

		uniform_real_distribution<float> fRandom(0.0f, 1.0f);
		Vector2 v(fRandom(m_rng), fRandom(m_rng));	// 随机2D向量

		// 点光源，向全球方向发射光子
		NXPhoton photon;
		photon.position = pLight->Position;
		photon.direction = UniformSampleSphere(v);	// 随机方向 theta phi->x y z
		photon.power = pLight->Intensity * fEachLightPhotonsInv;

		Ray photonRay(photon.position, photon.direction);
		NXIntersectionInfo isect;

		// 和场景进行迭代，发生交互
		if (NXIntersection::GetInstance()->RayIntersect(pScene, photonRay, isect))
		{
			
		}
	}
}

void NXPBRTRenderer::DrawPhotonMapping(const shared_ptr<NXScene>& pScene, const NXPBRTCamera& cameraInfo, const NXPBRTImage& imageInfo)
{
	float fAspectRatio = (float)cameraInfo.iViewWidth / (float)cameraInfo.iViewHeight;
	Matrix mxProj = XMMatrixPerspectiveFovLH(cameraInfo.fFovAngleY * DEGTORAD, fAspectRatio, 0.1f, 1000.0f);
	Matrix mxViewInv = Matrix(XMMatrixLookToLH(cameraInfo.position, cameraInfo.direction, cameraInfo.up)).Invert();

	m_outputImage.resize(cameraInfo.iViewWidth * cameraInfo.iViewHeight);

	for (int i = 0; i < cameraInfo.iViewWidth; i++)
	{
		for (int j = 0; j < cameraInfo.iViewHeight; j++)
		{
			for (int k = 0; k < imageInfo.iEachPixelSimples; k++)
			{
				uniform_real_distribution<float> rand(0.0f, 1.0f);
				float jitX = rand(m_rng);
				float jitY = rand(m_rng);

				// 屏幕像素坐标
				Vector2 fPixel((float)i + jitX, (float)j + jitY);

				Ray rayView;
				rayView.direction = Vector3(
					(fPixel.x * 2.0f / cameraInfo.iViewWidth - 1.0f) / mxProj._11,
					(1.0f - fPixel.y * 2.0f / cameraInfo.iViewHeight) / mxProj._22,
					1.0f);
				rayView.position = Vector3(0.0f);

				Ray rayWorld(Vector3::Transform(rayView.position, mxViewInv),
					Vector3::TransformNormal(rayView.position, mxViewInv));

				m_outputImage[i * cameraInfo.iViewWidth + j] += DrawPhotonMappingPerSample(pScene, rayWorld);
			}
		}
	}
}

Vector3 NXPBRTRenderer::DrawPhotonMappingPerSample(const shared_ptr<NXScene>& pScene, const Ray& rayWorld)
{
	NXIntersectionInfo isect;
	NXIntersection::GetInstance()->RayIntersect(pScene, rayWorld, isect);

	return Vector3(0.0f);
}

