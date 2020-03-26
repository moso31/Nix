#include "NXPBRTRenderer.h"
#include "NXScene.h"
#include "NXIntersection.h"

NXPBRTRenderer::NXPBRTRenderer()
{
	int seed = 0;	// can be changed as time.
	m_rng = default_random_engine(seed);
}

void NXPBRTRenderer::GeneratePhotonMap(const shared_ptr<NXScene>& pScene, const NXPhotonMap& photonMapInfo)
{
	for (int i = 0; i < photonMapInfo.Photons; i++)
	{

	}

	vector<float> intensityWeights;
	float fIntensitySum = 0.0f;
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
	{
		Vector3 lightIntensity = (*it)->Intensity;
		fIntensitySum += (lightIntensity.x + lightIntensity.y + lightIntensity.z) / 3.0f;
		intensityWeights.push_back(fIntensitySum);
	}

	uniform_real_distribution<float> rand(0.0f, fIntensitySum);
	float fRandom = rand(m_rng);
	for (int i = pLights.size(); i >= 0; i--)
	{
		if (fRandom < pLights[i])
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
				uniform_real_distribution<float> rand(0.0f, 0.999999999f);
				float jitX = rand(m_rng);
				float jitY = rand(m_rng);

				// ÆÁÄ»ÏñËØ×ø±ê
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

