#include "NXShadowMapRenderer.h"
#include "NXGlobalDefinitions.h"
#include "NXGlobalBuffers.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXResourceManager.h"
#include "NXAllocatorManager.h"
#include "NXConverter.h"

#include "NXScene.h"
#include "NXPBRLight.h"
#include "NXPrimitive.h"
#include "NXCamera.h"

NXShadowMapRenderer::NXShadowMapRenderer(NXScene* pScene) :
	m_pScene(pScene),
	m_cascadeCount(4),
	m_shadowMapRTSize(2048),
	m_shadowDistance(300.0f),
	m_cascadeExponentScale(2.0f),
	m_cascadeTransitionScale(0.1f),
	m_depthBias(100)
{
}

NXShadowMapRenderer::~NXShadowMapRenderer()
{
}

void NXShadowMapRenderer::InitShadowMapDepthTex()
{
	m_pShadowMapDepth = NXResourceManager::GetInstance()->GetTextureManager()->CreateRenderTexture2DArray("Shadow DepthZ RT", DXGI_FORMAT_R32_TYPELESS, m_shadowMapRTSize, m_shadowMapRTSize, m_cascadeCount, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, false);
	m_pShadowMapDepth->SetViews(1, 0, m_cascadeCount, 0);
	for (UINT i = 0; i < m_cascadeCount; i++)
		m_pShadowMapDepth->SetDSV(i, i, 1);	// DSV 单张切片（每次写cascade深度 只写一片）
	m_pShadowMapDepth->SetSRV(0, 0, m_cascadeCount); // SRV 读取整个纹理数组（ShadowTest时使用）
}

void NXShadowMapRenderer::SetupInternal()
{
	SetShaderFilePath("Shader\\ShadowMap.fx");
	SetRootParams(2, 0);
	SetStaticRootParamCBV(0, 0, &g_cbObject.GetFrameGPUAddresses()); // b0
	SetStaticRootParamCBV(1, 2, &g_cbShadowTest.GetFrameGPUAddresses()); // b2

	InitPSO();

	SetCascadeCount(m_cascadeCount);
	SetShadowDistance(m_shadowDistance);
	SetCascadeTransitionScale(m_cascadeTransitionScale);
	SetDepthBias(m_depthBias);

	for (int j = 0; j < 8; j++)
	{
		g_cbDataShadowTest.frustumParams[j] = Vector4(0.0f);
	}
}

void NXShadowMapRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	NX12Util::BeginEvent(pCmdList, "Shadow Map");

	NXRendererPass::RenderSetTargetAndState(pCmdList);
	NXRendererPass::RenderBefore(pCmdList);

	g_cbDataShadowTest.test_transition = m_test_transition;

	for (auto pLight : m_pScene->GetPBRLights())
	{
		if (pLight->GetType() == NXLightTypeEnum::NXLight_Distant)
		{
			NXPBRDistantLight* pDirLight = static_cast<NXPBRDistantLight*>(pLight);
			RenderCSMPerLight(pCmdList, pDirLight);
		}
	}

	NX12Util::EndEvent(pCmdList);
}

void NXShadowMapRenderer::RenderSingleObject(ID3D12GraphicsCommandList* pCmdList, NXRenderableObject* pRenderableObject)
{
	NXPrimitive* pPrimitive = pRenderableObject->IsPrimitive();
	if (pPrimitive)
	{
		pPrimitive->Update(pCmdList);

		for (UINT i = 0; i < pPrimitive->GetSubMeshCount(); i++)
		{
			NXSubMeshBase* pSubmesh = pPrimitive->GetSubMesh(i);
			pSubmesh->Render(pCmdList);
		}
	}

	for (auto pChildObject : pRenderableObject->GetChilds())
	{
		NXRenderableObject* pChildRenderableObject = pChildObject->IsRenderableObject();
		if (pChildRenderableObject)
			RenderSingleObject(pCmdList, pChildRenderableObject);
	}
}

void NXShadowMapRenderer::Release()
{
}

Ntr<NXTexture2DArray> NXShadowMapRenderer::GetShadowMapDepthTex()
{
	return m_pShadowMapDepth;
}

void NXShadowMapRenderer::SetCascadeCount(UINT value)
{
	m_cascadeCount = value;
	g_cbDataShadowTest.cascadeCount = (float)m_cascadeCount;
}

void NXShadowMapRenderer::SetDepthBias(int value)
{
	m_depthBias = value;
	g_cbDataShadowTest.depthBias = m_depthBias;
}

void NXShadowMapRenderer::SetShadowDistance(float value)
{
	m_shadowDistance = value;
	g_cbDataShadowTest.shadowDistance = m_shadowDistance;
}

void NXShadowMapRenderer::SetCascadeTransitionScale(float value)
{
	m_cascadeTransitionScale = value;
	g_cbDataShadowTest.cascadeTransitionScale = m_cascadeTransitionScale;
}

void NXShadowMapRenderer::RenderCSMPerLight(ID3D12GraphicsCommandList* pCmdList, NXPBRDistantLight* pDirLight)
{
	Vector3 lightDirection = pDirLight->GetDirection();
	lightDirection = lightDirection.IsZero() ? Vector3(0.0f, 0.0f, 1.0f) : lightDirection;
	lightDirection.Normalize();

	NXCamera* pCamera = m_pScene->GetMainCamera();
	Vector3 cameraPosition = pCamera->GetTranslation();
	Vector3 cameraDirection = pCamera->GetForward();

	Matrix mxCamViewProjInv = pCamera->GetViewProjectionInverseMatrix();
	Matrix mxCamProjInv = pCamera->GetProjectionInverseMatrix();

	float zNear = pCamera->GetZNear();
	float zLength = m_shadowDistance - zNear;

	Matrix mxCamProj = pCamera->GetProjectionMatrix();

	float expScale = 1.0f;
	float sumInv = 1.0f;
	for (UINT i = 1; i < m_cascadeCount; i++)
	{
		expScale *= m_cascadeExponentScale;
		sumInv += expScale;
	}
	sumInv = 1.0f / sumInv;

	expScale = 1.0f;
	float percentage = 0.0f;
	float zLastCascadeTransitionLength = 0.0f;
	for (UINT i = 0; i < m_cascadeCount; i++)
	{
		// 按等比数列划分 cascade
		float percentageOffset = expScale * sumInv;
		expScale *= m_cascadeExponentScale;

		float zCascadeNear = zNear + percentage * zLength;
		percentage += percentageOffset;
		float zCascadeFar  = zNear + percentage * zLength;
		float zCascadeLength = zCascadeFar - zCascadeNear;

		zCascadeNear -= zLastCascadeTransitionLength;

		// 此数值 用于 cascade 之间的平滑过渡
		zLastCascadeTransitionLength = zCascadeLength * m_cascadeTransitionScale;

		zCascadeLength += zLastCascadeTransitionLength;

		g_cbDataShadowTest.frustumParams[i] = Vector4(zCascadeFar, zLastCascadeTransitionLength, 0.0f, 0.0f);

		float zCascadeNearProj = (zCascadeNear * mxCamProj._33 + mxCamProj._43) / zCascadeNear;
		float zCascadeFarProj  = (zCascadeFar  * mxCamProj._33 + mxCamProj._43) / zCascadeFar;

		// 计算各层 cascade 的 Frustum (view space)
		Vector3 viewFrustum[8];
		viewFrustum[0] = Vector3::Transform(Vector3(-1.0f, -1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[1] = Vector3::Transform(Vector3(-1.0f,  1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[2] = Vector3::Transform(Vector3( 1.0f, -1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[3] = Vector3::Transform(Vector3( 1.0f,  1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[4] = Vector3::Transform(Vector3(-1.0f, -1.0f, zCascadeFarProj), mxCamProjInv);
		viewFrustum[5] = Vector3::Transform(Vector3(-1.0f,  1.0f, zCascadeFarProj), mxCamProjInv);
		viewFrustum[6] = Vector3::Transform(Vector3( 1.0f, -1.0f, zCascadeFarProj), mxCamProjInv);
		viewFrustum[7] = Vector3::Transform(Vector3( 1.0f,  1.0f, zCascadeFarProj), mxCamProjInv);

		// 计算 Frustum 的外接球
		float a2 = (viewFrustum[3] - viewFrustum[0]).LengthSquared();
		float b2 = (viewFrustum[7] - viewFrustum[4]).LengthSquared();
		float delta = zCascadeLength * 0.5f + (a2 - b2) / (8.0f * zCascadeLength);

		// 计算 外接球 的 球心，view space 和 world space 都要。
		// zCascadeDistance: 当前 cascade 中 Near平面中心点 到 frustum 外接球心 的距离
		float zCascadeDistance = zCascadeLength - delta;
		Vector3 sphereCenterVS = Vector3(0.0f, 0.0f, zCascadeNear + zCascadeDistance);
		Vector3 sphereCenterWS = cameraPosition + cameraDirection * sphereCenterVS.z;

		// 计算 外接球 的 半径
		float sphereRadius = sqrtf(zCascadeDistance * zCascadeDistance + (a2 * 0.25f));

		Vector3 shadowMapEye = Vector3(0.0f);
		Vector3 shadowMapAt = -lightDirection;
		Vector3 shadowMapUp = Vector3(0.0f, 1.0f, 0.0f);
		Matrix mxShadowView = XMMatrixLookAtLH(shadowMapEye, shadowMapAt, shadowMapUp);

		float cascadeBound = sphereRadius * 2.0f;
		float worldUnitsPerPixel = cascadeBound / m_shadowMapRTSize;	

		// "LS" = "light space" = shadow camera ortho space.
		Vector3 sphereCenterLS = Vector3::Transform(sphereCenterWS, mxShadowView);
		sphereCenterLS -= Vector3(fmodf(sphereCenterLS.x, worldUnitsPerPixel), fmodf(sphereCenterLS.y, worldUnitsPerPixel), 0.0f);
		sphereCenterWS = Vector3::Transform(sphereCenterLS, mxShadowView.Invert());

		Vector3 sceneCenter = m_pScene->GetBoundingSphere().Center;
		float sceneRadius = m_pScene->GetBoundingSphere().Radius;
		float backDistance = Vector3::Distance(sceneCenter, sphereCenterWS) + sphereRadius;
		shadowMapEye = sphereCenterWS - lightDirection * backDistance;
		shadowMapAt = sphereCenterWS;
		shadowMapUp = Vector3(0.0f, 1.0f, 0.0f);
		mxShadowView = XMMatrixLookAtLH(shadowMapEye, shadowMapAt, shadowMapUp);

		// 2022.5.15 目前平行光 proj 的矩阵方案，对z的范围取值很保守。可以改进
		Matrix mxShadowProj = XMMatrixOrthographicOffCenterLH(-sphereRadius, sphereRadius, -sphereRadius, sphereRadius, 0.0f, backDistance * 2.0f);

		// 更新当前 cascade 层 的 ShadowMap view proj 绘制矩阵
		m_cbDataCSMViewProj[i].view = mxShadowView.Transpose();
		m_cbDataCSMViewProj[i].projection = mxShadowProj.Transpose();
		g_cbDataShadowTest.view[i] = mxShadowView.Transpose();
		g_cbDataShadowTest.projection[i] = mxShadowProj.Transpose();
		m_cbCSMViewProj[i].Update(m_cbDataCSMViewProj[i]);

		pCmdList->ClearDepthStencilView(m_pShadowMapDepth->GetDSV(i), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0x0, 0, nullptr);
		pCmdList->OMSetRenderTargets(0, nullptr, false, &m_pShadowMapDepth->GetDSV(i));
		pCmdList->SetGraphicsRootConstantBufferView(1, m_cbCSMViewProj[i].CurrentGPUAddress());

		// 更新当前 cascade 层 的 ShadowMap world 绘制矩阵，并绘制
		for (auto pRenderableObj : m_pScene->GetRenderableObjects())
		{
			RenderSingleObject(pCmdList, pRenderableObj);
		}
	}

	// Shadow Test
	g_cbShadowTest.Update(g_cbDataShadowTest);
}
