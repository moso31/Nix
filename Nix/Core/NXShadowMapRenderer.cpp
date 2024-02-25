#include "NXShadowMapRenderer.h"
#include "Global.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXResourceManager.h"
#include "NXAllocatorManager.h"

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
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\ShadowMap.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ShadowMap.fx", "PS", pPSBlob.Get());

	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	rootParams.push_back(NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL)); // b0
	m_pRootSig = NX12Util::CreateRootSignature(g_pDevice.Get(), rootParams);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = { NXGlobalInputLayout::layoutPT, 1 };
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<true, true, D3D12_COMPARISON_FUNC_LESS>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 0;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN; // shadowmap 不需要RTV
	psoDesc.DSVFormat = m_pShadowMapDepth->GetFormat();
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	g_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));

	m_pShadowMapDepth = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DArray("Shadow DepthZ RT", DXGI_FORMAT_R32_TYPELESS, m_shadowMapRTSize, m_shadowMapRTSize, m_cascadeCount, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	for (UINT i = 0; i < m_cascadeCount; i++)
		m_pShadowMapDepth->AddDSV(i, 1);	// DSV 单张切片（每次写cascade深度 只写一片）
	m_pShadowMapDepth->AddSRV(0, m_cascadeCount); // SRV 读取整个纹理数组（ShadowTest时使用）

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->Alloc(ResourceType_Upload, m_cbShadowMapObject.Get(i));

	SetCascadeCount(m_cascadeCount);
	SetShadowDistance(m_shadowDistance);
	SetCascadeTransitionScale(m_cascadeTransitionScale);
	SetDepthBias(m_depthBias);

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			NXGlobalBufferManager::m_cbDataShadowTest.Get(i).data.frustumParams[j] = Vector4(0.0f);
		}
	}
}

void NXShadowMapRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Shadow Map");

	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	NXGlobalBufferManager::m_cbDataShadowTest.Current().data.test_transition = m_test_transition;

	for (auto pLight : m_pScene->GetPBRLights())
	{
		if (pLight->GetType() == NXLightTypeEnum::NXLight_Distant)
		{
			NXPBRDistantLight* pDirLight = static_cast<NXPBRDistantLight*>(pLight);
			UpdateShadowMapBuffer(pDirLight);
		}
	}

	NX12Util::EndEvent();
}

void NXShadowMapRenderer::RenderSingleObject(NXRenderableObject* pRenderableObject)
{
	NXPrimitive* pPrimitive = pRenderableObject->IsPrimitive();
	if (pPrimitive)
	{
		m_cbShadowMapObject.Current().data.world = pPrimitive->GetWorldMatrix().Transpose();
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->UpdateData(m_cbShadowMapObject.Current());

		m_pCommandList->SetGraphicsRootConstantBufferView(0, m_cbShadowMapObject.Current().GPUVirtualAddr);

		for (UINT i = 0; i < pPrimitive->GetSubMeshCount(); i++)
		{
			NXSubMeshBase* pSubmesh = pPrimitive->GetSubMesh(i);
			pSubmesh->Render(m_pCommandList.Get());
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
}

Ntr<NXTexture2DArray> NXShadowMapRenderer::GetShadowMapDepthTex()
{
	return m_pShadowMapDepth;
}

void NXShadowMapRenderer::SetCascadeCount(UINT value)
{
	m_cascadeCount = value;
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		NXGlobalBufferManager::m_cbDataShadowTest.Get(i).data.cascadeCount = (float)m_cascadeCount;
	}
}

void NXShadowMapRenderer::SetDepthBias(int value)
{
	m_depthBias = value;
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		NXGlobalBufferManager::m_cbDataShadowTest.Get(i).data.depthBias = (float)m_depthBias;
	}
}

void NXShadowMapRenderer::SetShadowDistance(float value)
{
	m_shadowDistance = value;
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		NXGlobalBufferManager::m_cbDataShadowTest.Get(i).data.shadowDistance = m_shadowDistance;
	}
}

void NXShadowMapRenderer::SetCascadeTransitionScale(float value)
{
	m_cascadeTransitionScale = value;
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		NXGlobalBufferManager::m_cbDataShadowTest.Get(i).data.cascadeTransitionScale = m_cascadeTransitionScale;
	}
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

		NXGlobalBufferManager::m_cbDataShadowTest.Current().data.frustumParams[i] = Vector4(zCascadeFar, zLastCascadeTransitionLength, 0.0f, 0.0f);

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
		m_cbShadowMapObject.Current().data.view = mxShadowView.Transpose();
		m_cbShadowMapObject.Current().data.projection = mxShadowProj.Transpose();
		NXGlobalBufferManager::m_cbDataShadowTest.Current().data.view[i] = m_cbShadowMapObject.Current().data.view;
		NXGlobalBufferManager::m_cbDataShadowTest.Current().data.projection[i] = m_cbShadowMapObject.Current().data.projection;

		m_pCommandList->ClearDepthStencilView(m_pShadowMapDepth->GetDSV(i), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0x0, 0, nullptr);
		m_pCommandList->OMSetRenderTargets(0, nullptr, false, &m_pShadowMapDepth->GetDSV(i));

		// 更新当前 cascade 层 的 ShadowMap world 绘制矩阵，并绘制
		for (auto pRenderableObj : m_pScene->GetRenderableObjects())
		{
			RenderSingleObject(pRenderableObj);
		}
	}

	// Shadow Test
	NXAllocatorManager::GetInstance()->GetCBufferAllocator()->UpdateData(NXGlobalBufferManager::m_cbDataShadowTest.Current());
}
