#include "Renderer.h"
#include "NXTimer.h"
#include "NXGlobalDefinitions.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "NXEvent.h"
#include "NXResourceManager.h"
#include "NXResourceReloader.h"
#include "NXRenderStates.h"
#include "NXGUI.h"
#include "NXTexture.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXDepthPrepass.h"
#include "NXSimpleSSAO.h"
#include "NXAllocatorManager.h"
#include "NXSubMeshGeometryEditor.h"

Renderer::Renderer(const Vector2& rtSize) :
	m_bRenderGUI(true),
	m_viewRTSize(rtSize)
{
}

void Renderer::Init()
{
	// �����¼�
	InitEvents();

	// ��������DX12��Դ������
	NXAllocatorManager::GetInstance()->Init();

	NXGlobalInputLayout::Init();
	NXGlobalBuffer::Init();

	NXTexture::Init();

	NX12Util::CreateCommands(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandQueue.GetAddressOf(), m_pCommandAllocator.GetAddressOf(), m_pCommandList.GetAddressOf());

	// ��Ⱦ��
	InitRenderer();

	NXSubMeshGeometryEditor::GetInstance()->Init(NXGlobalDX::GetDevice());

	m_scene = new NXScene();

	NXResourceManager::GetInstance()->GetMaterialManager()->Init();

	NXResourceManager::GetInstance()->GetMeshManager()->Init(m_scene);
	NXResourceManager::GetInstance()->GetCameraManager()->SetWorkingScene(m_scene);
	NXResourceManager::GetInstance()->GetLightManager()->SetWorkingScene(m_scene);

	m_scene->Init();

	auto pCubeMap = m_scene->GetCubeMap();

	m_pBRDFLut = new NXBRDFLut();
	m_pBRDFLut->Init();

	//m_pDepthPrepass = new NXDepthPrepass(m_scene);
	//m_pDepthPrepass->Init();

	m_pDepthRenderer = new NXDepthRenderer();
	m_pDepthRenderer->Init();

	m_pGBufferRenderer = new NXGBufferRenderer(m_scene);
	m_pGBufferRenderer->Init();

	//m_pSSAO = new NXSimpleSSAO();
	//m_pSSAO->Init();

	m_pShadowMapRenderer = new NXShadowMapRenderer(m_scene);
	m_pShadowMapRenderer->Init();

	m_pShadowTestRenderer = new NXShadowTestRenderer();
	m_pShadowTestRenderer->Init();

	m_pDeferredRenderer = new NXDeferredRenderer(m_scene, m_pBRDFLut);
	m_pDeferredRenderer->Init();

	m_pSubSurfaceRenderer = new NXSubSurfaceRenderer(m_scene);
	m_pSubSurfaceRenderer->Init();

	//m_pForwardRenderer = new NXForwardRenderer(m_scene, m_pBRDFLut);
	//m_pForwardRenderer->Init();

	//m_pDepthPeelingRenderer = new NXDepthPeelingRenderer(m_scene, m_pBRDFLut);
	//m_pDepthPeelingRenderer->Init();

	m_pSkyRenderer = new NXSkyRenderer(m_scene);
	m_pSkyRenderer->Init();

	m_pColorMappingRenderer = new NXColorMappingRenderer();
	m_pColorMappingRenderer->Init();

	m_pDebugLayerRenderer = new NXDebugLayerRenderer(m_pShadowMapRenderer);
	m_pDebugLayerRenderer->Init(m_viewRTSize);

	m_pEditorObjectRenderer = new NXEditorObjectRenderer(m_scene);
	m_pEditorObjectRenderer->Init();
}

void Renderer::OnResize(const Vector2& rtSize)
{
	m_viewRTSize = rtSize;

	NXResourceManager::GetInstance()->GetTextureManager()->ResizeCommonRT(m_viewRTSize);
	//m_pDepthPrepass->OnResize(m_viewRTSize);
	//m_pSSAO->OnResize(m_viewRTSize);
	//m_pDepthPeelingRenderer->OnResize(m_viewRTSize);
	m_pDebugLayerRenderer->OnResize(m_viewRTSize);
	m_pEditorObjectRenderer->OnResize(m_viewRTSize);

	m_scene->OnResize(rtSize);
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene, this);
	m_pGUI->Init();
}

void Renderer::InitRenderer()
{
	NXResourceManager::GetInstance()->GetTextureManager()->InitCommonTextures();

	// 2024.4.9 ��InitCommonRT �� OnResize �ᵼ�²���Ҫ���ظ�����RT����Ҫ�Ż���
	NXResourceManager::GetInstance()->GetTextureManager()->InitCommonRT(m_viewRTSize);
}
 
void Renderer::InitEvents()
{
	NXEventKeyDown::GetInstance()->AddListener(std::bind(&Renderer::OnKeyDown, this, std::placeholders::_1));
}

void Renderer::ResourcesReloading()
{
	NXResourceManager::GetInstance()->OnReload();
	NXResourceReloader::GetInstance()->OnReload();
}

void Renderer::Update()
{
	UpdateGUI();
	UpdateSceneData();
}

void Renderer::UpdateGUI()
{
	m_pGUI->ExecuteDeferredCommands();
}

void Renderer::UpdateSceneData()
{
	UpdateTime();

	// ���³���Scripts��ʵ��������Scripts����ָ�������Transform��
	m_scene->UpdateScripts();

	// ����Transform
	m_scene->UpdateTransform();
	m_scene->UpdateTransformOfEditorObjects();

	// ����Camera�ĳ����������ݣ�VP�����۾�λ�ã�
	m_scene->UpdateCamera();

	m_scene->UpdateLightData();

	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->Update();
	}

	//m_pSSAO->Update();
}

void Renderer::UpdateTime()
{
	size_t globalTime = NXGlobalApp::Timer->GetGlobalTime();
	float fGlobalTime = globalTime / 1000.0f;

	NXGlobalBuffer::cbObject.Current().globalData.time = fGlobalTime;
}

void Renderer::RenderFrame()
{
	m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

	auto& pSceneRT = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting0);
	if (pSceneRT.IsNull()) return;

	NX12Util::BeginEvent(m_pCommandList.Get(), "Render Scene");

	// �����ӿ�
	auto vpCamera = NX12Util::ViewPort(m_viewRTSize.x, m_viewRTSize.y);
	m_pCommandList->RSSetViewports(1, &vpCamera);
	m_pCommandList->ClearRenderTargetView(pSceneRT->GetRTV(), Colors::Black, 0, nullptr);

	//m_pDepthPrepass->Render();

	// GBuffer
	m_pGBufferRenderer->Render(m_pCommandList.Get());

	// Depth Copy 2023.10.26
	// Burley SSS ����Ҫ�� Depth ��ģ�建��󶨵�output������Ҫ�����Ϣ��Ϊinput����ͻᵼ����Դ�󶨳�ͻ��
	// �������� Copy һ�ݼ�¼�� GBuffer Ϊֹ�� Depth �� DepthR32��
	// ������Ҫ������Դ�󶨳�ͻ���������ʹ�ø��Ƶ�������Ϊinput��
	// ��δ��������Ҫ�������� Hi-Z��
	m_pDepthRenderer->Render(m_pCommandList.Get());

	// Shadow Map
	auto vpShadow = NX12Util::ViewPort(2048, 2048);
	m_pCommandList->RSSetViewports(1, &vpShadow);
	m_pShadowMapRenderer->Render(m_pCommandList.Get());
	m_pCommandList->RSSetViewports(1, &vpCamera);
	m_pShadowTestRenderer->SetShadowMapDepth(m_pShadowMapRenderer->GetShadowMapDepthTex());
	m_pShadowTestRenderer->Render(m_pCommandList.Get());

	// Deferred opaque shading
	m_pDeferredRenderer->Render(m_pCommandList.Get());

	// Burley SSS (2015)
	m_pSubSurfaceRenderer->Render(m_pCommandList.Get());

	// CubeMap
	m_pSkyRenderer->Render(m_pCommandList.Get());

	// Forward translucent shading
	// 2023.8.20 ǰ����Ⱦ��ʱͣ�ã��� 3S �����
	//m_pForwardRenderer->Render();
	//m_pDepthPeelingRenderer->Render(bSSSEnable);

	//// SSAO
	//m_pSSAO->Render(pSRVNormal, pSRVPosition, pSRVDepthPrepass);

	// post processing
	m_pColorMappingRenderer->Render(m_pCommandList.Get());

	// ���Ʊ༭������
	m_pEditorObjectRenderer->Render(m_pCommandList.Get());

	// ���Ƶ�����Ϣ�㣨����еĻ���
	m_pDebugLayerRenderer->Render(m_pCommandList.Get());

	// �ж� GUIView ʹ����������RT ��Ϊ Input
	bool bEnableDebugLayer = m_pDebugLayerRenderer->GetEnableDebugLayer();
	m_pFinalRT = bEnableDebugLayer ? m_pDebugLayerRenderer->GetDebugLayerTex() :
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing);

	NX12Util::EndEvent();

	m_pCommandList->Close();
	ID3D12CommandList* pCmdLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(1, pCmdLists);
}

void Renderer::RenderGUI(D3D12_CPU_DESCRIPTOR_HANDLE swapChainRTV)
{
	if (m_bRenderGUI) m_pGUI->Render(m_pFinalRT, swapChainRTV);
}

void Renderer::Release()
{
	SafeRelease(m_pEditorObjectRenderer);
	SafeRelease(m_pDebugLayerRenderer);
	SafeRelease(m_pGUI);

	//SafeRelease(m_pSSAO);
	SafeDelete(m_pDepthPrepass);

	SafeRelease(m_pDepthRenderer);
	SafeRelease(m_pGBufferRenderer);
	SafeRelease(m_pShadowMapRenderer);
	SafeRelease(m_pShadowTestRenderer);
	SafeRelease(m_pDeferredRenderer);
	SafeRelease(m_pSubSurfaceRenderer);
	//SafeRelease(m_pForwardRenderer);
	//SafeRelease(m_pDepthPeelingRenderer);
	SafeRelease(m_pSkyRenderer);
	SafeRelease(m_pColorMappingRenderer);

	SafeRelease(m_pBRDFLut);
	SafeRelease(m_scene);
}

void Renderer::ClearAllPSResources()
{
}

void Renderer::DrawDepthPrepass()
{
}

void Renderer::OnKeyDown(NXEventArgKey eArg)
{
	//if (eArg.VKey == 'H')
	//{
	//	m_bRenderGUI = !m_bRenderGUI;
	//}
}
