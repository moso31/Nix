#include "Renderer.h"
#include "NXTimer.h"
#include "NXGlobalBuffers.h"
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
#include "NXPSOManager.h"

Renderer::Renderer(const Vector2& rtSize) :
	m_bRenderGUI(true),
	m_viewRTSize(rtSize)
{
}

void Renderer::Init()
{
	// �����¼�
	InitEvents();

	// ��ʼ����Դ
	InitGlobalResources();

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

	// 2024.7.13 ˼·ת�䣬��Pass������Ⱦ����ͼ��һ���ڵ�
	// ����ʵʱ�޸���Щ����������ʹ����Щ����ʹ���ĸ�Shader��State�����õȡ�
	// ��Ȼ����û��RenderGraph�������Ի������˼��ȥ��ƴ��롣
	m_pShadowTestRenderer = new NXShadowTestRenderer();
	m_pShadowTestRenderer->SetPassName("Shadow Test");
	m_pShadowTestRenderer->RegisterTextures(2, 1);
	m_pShadowTestRenderer->SetInputTex(0, NXCommonRT_DepthZ);
	m_pShadowTestRenderer->SetInputTex(1, m_pShadowMapRenderer->GetShadowMapDepthTex());
	m_pShadowTestRenderer->SetOutputRT(0, NXCommonRT_ShadowTest);
	m_pShadowTestRenderer->SetShaderFilePath("Shader\\ShadowTest.fx");
	m_pShadowTestRenderer->SetRasterizerState(NXRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, 0, 0, 1000.0f>::Create());
	m_pShadowTestRenderer->SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());
	m_pShadowTestRenderer->SetRootParams(3, 2); 
	m_pShadowTestRenderer->SetStaticRootParamCBV(0, &g_cbObject.GetFrameGPUAddresses());
	m_pShadowTestRenderer->SetStaticRootParamCBV(1, &g_cbCamera.GetFrameGPUAddresses());
	m_pShadowTestRenderer->SetStaticRootParamCBV(2, &g_cbShadowTest.GetFrameGPUAddresses());
	m_pShadowTestRenderer->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	m_pShadowTestRenderer->InitPSO();

	m_pDeferredRenderer = new NXDeferredRenderer(m_scene, m_pBRDFLut);
	m_pDeferredRenderer->Init();

	m_pSubSurfaceRenderer = new NXSubSurfaceRenderer();
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
	m_pDebugLayerRenderer->Init();

	m_pEditorObjectRenderer = new NXEditorObjectRenderer(m_scene);
	m_pEditorObjectRenderer->Init();

	InitGUI();
}

void Renderer::OnResize(const Vector2& rtSize)
{
	m_viewRTSize = rtSize;

	NXResourceManager::GetInstance()->GetTextureManager()->ResizeCommonRT(m_viewRTSize);
	m_pDeferredRenderer->OnResize();
	m_pDepthRenderer->OnResize();
	m_pSubSurfaceRenderer->OnResize();
	m_pSkyRenderer->OnResize();
	//m_pDepthPrepass->OnResize(m_viewRTSize);
	//m_pSSAO->OnResize(m_viewRTSize);
	//m_pDepthPeelingRenderer->OnResize(m_viewRTSize);
	m_pShadowTestRenderer->OnResize();
	m_pColorMappingRenderer->OnResize();
	m_pDebugLayerRenderer->OnResize(m_viewRTSize);
	m_pEditorObjectRenderer->OnResize();

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

void Renderer::InitGlobalResources()
{
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		m_pCommandAllocator.Get(i) = NX12Util::CreateCommandAllocator(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		std::wstring strCmdAllocatorName(L"Main Renderer Command Allocator" + std::to_wstring(i));
		m_pCommandAllocator.Get(i)->SetName(strCmdAllocatorName.c_str());

		m_pCommandList.Get(i) = NX12Util::CreateGraphicsCommandList(NXGlobalDX::GetDevice(), m_pCommandAllocator.Get(i).Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		std::wstring strCmdListName(L"Main Renderer Command List" + std::to_wstring(i));
		m_pCommandList.Get(i)->SetName(strCmdListName.c_str());
	}

	NXAllocatorManager::GetInstance()->Init();
	NXGlobalInputLayout::Init();

	// PSOManager
	NXPSOManager::GetInstance()->Init(NXGlobalDX::GetDevice(), NXGlobalDX::GetCmdQueue());
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

	NXAllocatorManager::GetInstance()->Update();
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
	g_cbDataObject.globalData.time = NXGlobalApp::Timer->GetGlobalTimeSeconds();
}

void Renderer::RenderFrame()
{
	auto& pCommandAllocator = m_pCommandAllocator.Current();
	auto& pCommandList = m_pCommandList.Current();

	pCommandAllocator->Reset();
	pCommandList->Reset(pCommandAllocator.Get(), nullptr);

	NX12Util::BeginEvent(pCommandList.Get(), "Render Scene");

	// �����ӿ�
	auto vpCamera = NX12Util::ViewPort(m_viewRTSize.x, m_viewRTSize.y);
	pCommandList->RSSetViewports(1, &vpCamera);
	pCommandList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));

	ID3D12DescriptorHeap* ppHeaps[] = { NXShVisDescHeap->GetDescriptorHeap() };
	pCommandList->SetDescriptorHeaps(1, ppHeaps);

	//m_pDepthPrepass->Render();

	// GBuffer
	m_pGBufferRenderer->Render(pCommandList.Get());

	// Depth Copy 2023.10.26
	// Burley SSS ����Ҫ�� Depth ��ģ�建��󶨵�output������Ҫ�����Ϣ��Ϊinput����ͻᵼ����Դ�󶨳�ͻ��
	// �������� Copy һ�ݼ�¼�� GBuffer Ϊֹ�� Depth �� DepthR32��
	// ������Ҫ������Դ�󶨳�ͻ���������ʹ�ø��Ƶ�������Ϊinput��
	// ��δ��������Ҫ�������� Hi-Z��
	m_pDepthRenderer->Render(pCommandList.Get());

	// Shadow Map
	auto vpShadow = NX12Util::ViewPort(2048, 2048);
	pCommandList->RSSetViewports(1, &vpShadow);
	pCommandList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpShadow));
	m_pShadowMapRenderer->Render(pCommandList.Get());
	pCommandList->RSSetViewports(1, &vpCamera);
	pCommandList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));
	m_pShadowTestRenderer->Render(pCommandList.Get());

	// Deferred opaque shading
	m_pDeferredRenderer->Render(pCommandList.Get());

	// Burley SSS (2015)
	m_pSubSurfaceRenderer->Render(pCommandList.Get());

	// CubeMap
	m_pSkyRenderer->Render(pCommandList.Get());

	// Forward translucent shading
	// 2023.8.20 ǰ����Ⱦ��ʱͣ�ã��� 3S �����
	//m_pForwardRenderer->Render();
	//m_pDepthPeelingRenderer->Render(bSSSEnable);

	//// SSAO
	//m_pSSAO->Render(pSRVNormal, pSRVPosition, pSRVDepthPrepass);

	// post processing
	m_pColorMappingRenderer->Render(pCommandList.Get());

	// ���Ʊ༭������
	m_pEditorObjectRenderer->Render(pCommandList.Get());

	// ���Ƶ�����Ϣ�㣨����еĻ���
	m_pDebugLayerRenderer->Render(pCommandList.Get());

	// �ж� GUIView ʹ����������RT ��Ϊ Input
	bool bEnableDebugLayer = m_pDebugLayerRenderer->GetEnableDebugLayer();
	m_pFinalRT = bEnableDebugLayer ? NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DebugLayer) :
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing);

	NX12Util::EndEvent(pCommandList.Get());

	pCommandList->Close();
	ID3D12CommandList* pCmdLists[] = { pCommandList.Get() };
	NXGlobalDX::GetCmdQueue()->ExecuteCommandLists(1, pCmdLists);

	// ����PSOManager״̬
	NXPSOManager::GetInstance()->FrameCleanup();
}

void Renderer::RenderGUI(const NXSwapChainBuffer& swapChainBuffer)
{
	if (m_bRenderGUI) m_pGUI->Render(m_pFinalRT, swapChainBuffer);
}

void Renderer::Release()
{
	SafeRelease(m_pEditorObjectRenderer);
	SafeRelease(m_pDebugLayerRenderer);
	SafeRelease(m_pGUI);

	//SafeRelease(m_pSSAO);
	//SafeDelete(m_pDepthPrepass);

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

	NXAllocatorManager::GetInstance()->Release();
	NXSubMeshGeometryEditor::GetInstance()->Release();
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
