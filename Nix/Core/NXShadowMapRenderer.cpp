#include "NXShadowMapRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXResourceManager.h"

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

void NXShadowMapRenderer::Init()
{
	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\ShadowMap.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ShadowMap.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<true, true, D3D11_COMPARISON_LESS>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create(0);
	m_pBlendState = NXBlendState<>::Create();

	m_pShadowMapDepth = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DArray("Shadow DepthZ RT", DXGI_FORMAT_R32_TYPELESS, m_shadowMapRTSize, m_shadowMapRTSize, m_cascadeCount, 1, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
	for (UINT i = 0; i < m_cascadeCount; i++)
		m_pShadowMapDepth->AddDSV(i, 1);	// DSV 单张切片（每次写cascade深度 只写一片）
	m_pShadowMapDepth->AddSRV(0, m_cascadeCount); // SRV 读取整个纹理数组（ShadowTest时使用）

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBufferShadowMapObject);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbShadowMapObject));


	SetCascadeCount(m_cascadeCount);
	SetShadowDistance(m_shadowDistance);
	SetCascadeTransitionScale(m_cascadeTransitionScale);
	SetDepthBias(m_depthBias);

	for (int i = 0; i < 8; i++) NXGlobalBufferManager::m_cbDataShadowTest.frustumParams[i] = Vector4(0.0f);
}

void NXShadowMapRenderer::Render()
{
	g_pUDA->BeginEvent(L"Shadow Map");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());


	NXGlobalBufferManager::m_cbDataShadowTest.test_transition = m_test_transition;

	for (auto pLight : m_pScene->GetPBRLights())
	{
		if (pLight->GetType() == NXLightTypeEnum::NXLight_Distant)
		{
			NXPBRDistantLight* pDirLight = static_cast<NXPBRDistantLight*>(pLight);
			UpdateShadowMapBuffer(pDirLight);
		}
	}

	g_pUDA->EndEvent();
}

void NXShadowMapRenderer::RenderSingleObject(NXRenderableObject* pRenderableObject)
{
	NXPrimitive* pPrimitive = pRenderableObject->IsPrimitive();
	if (pPrimitive)
	{
		m_cbDataShadowMapObject.world = pPrimitive->GetWorldMatrix().Transpose();
		g_pContext->UpdateSubresource(m_cbShadowMapObject.Get(), 0, nullptr, &m_cbDataShadowMapObject, 0, 0);

		g_pContext->VSSetConstantBuffers(0, 1, m_cbShadowMapObject.GetAddressOf());

		for (UINT i = 0; i < pPrimitive->GetSubMeshCount(); i++)
		{
			NXSubMeshBase* pSubmesh = pPrimitive->GetSubMesh(i);
			pSubmesh->Render();
		}
	}

	for (auto pChildObject : pRenderableObject->GetChilds())
	{
		NXRenderableObject* pChildRenderableObject = pChildObject->IsRenderableObject();
		if (pChildRenderableObject)
			RenderSingleObject(pChildRenderableObject);
	}
}

void NXShadowMapRenderer::Release()
{
	m_pShadowMapDepth->RemoveRef();
}

void NXShadowMapRenderer::SetCascadeCount(UINT value)
{
	m_cascadeCount = value;
	NXGlobalBufferManager::m_cbDataShadowTest.cascadeCount = (float)m_cascadeCount;
}

void NXShadowMapRenderer::SetDepthBias(int value)
{
	m_depthBias = value;
	NXGlobalBufferManager::m_cbDataShadowTest.depthBias = m_depthBias;
}

void NXShadowMapRenderer::SetShadowDistance(float value)
{
	m_shadowDistance = value;
	NXGlobalBufferManager::m_cbDataShadowTest.shadowDistance = value;
}

void NXShadowMapRenderer::SetCascadeTransitionScale(float value)
{
	m_cascadeTransitionScale = value;
	NXGlobalBufferManager::m_cbDataShadowTest.cascadeTransitionScale = value;
}

void NXShadowMapRenderer::UpdateShadowMapBuffer(NXPBRDistantLight* pDirLight)
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

		NXGlobalBufferManager::m_cbDataShadowTest.frustumParams[i] = Vector4(zCascadeFar, zLastCascadeTransitionLength, 0.0f, 0.0f);

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
		m_cbDataShadowMapObject.view = mxShadowView.Transpose();
		m_cbDataShadowMapObject.projection = mxShadowProj.Transpose();
		// 顺便在 ShadowTest 也保留一份
		NXGlobalBufferManager::m_cbDataShadowTest.view[i] = m_cbDataShadowMapObject.view;
		NXGlobalBufferManager::m_cbDataShadowTest.projection[i] = m_cbDataShadowMapObject.projection;

		auto pDSVShadowDepth = m_pShadowMapDepth->GetDSV(i);
		g_pContext->ClearDepthStencilView(pDSVShadowDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
		g_pContext->OMSetRenderTargets(0, nullptr, pDSVShadowDepth);

		// 更新当前 cascade 层 的 ShadowMap world 绘制矩阵，并绘制
		for (auto pRenderableObj : m_pScene->GetRenderableObjects())
		{
			RenderSingleObject(pRenderableObj);
		}
	}

	// Shadow Test
	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbShadowTest.Get(), 0, nullptr, &NXGlobalBufferManager::m_cbDataShadowTest, 0, 0);
}
