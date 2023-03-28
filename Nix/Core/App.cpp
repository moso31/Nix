#include "App.h"
#include "DirectResources.h"
#include "NXResourceManager.h"
#include "NXEvent.h"

App::App() :
	m_pRenderer(nullptr)
{

}

void App::Init()
{
	g_dxResources = new DirectResources();
	g_dxResources->InitDevice();

	m_pRenderer = new Renderer();
	m_pRenderer->Init();

	m_pRenderer->InitGUI();
}

void App::Reload()
{
	m_pRenderer->ResourcesReloading();

	m_pRenderer->PipelineReloading();
}

void App::Update()
{
	// ���³�������
	m_pRenderer->UpdateSceneData();
}

void App::Draw()
{
	m_pRenderer->RenderFrame();
	m_pRenderer->RenderGUI();

	// clear SRV.
	ID3D11ShaderResourceView* const pNullSRV[16] = { nullptr };
	g_pContext->PSSetShaderResources(0, 16, pNullSRV);

	//// Present() ���û�������Ⱦͼ��
	//// ��Present֮ǰ��������GPU�����е�����ȫ��ִ���꣬������ܻ������Ⱦ���⡣
	//// �������л�CubeMapSRV-Preview2D�����δ����Present֮ǰ��ʱ���ص��³����������
	//ComPtr<ID3D11Query> pQuery;
	//g_pDevice->CreateQuery(&CD3D11_QUERY_DESC(D3D11_QUERY_EVENT, 0), &pQuery);
	//g_pContext->End(pQuery.Get());

	//while (g_pContext->GetData(pQuery.Get(), nullptr, 0, 0) != S_OK)
	//{
	//}

	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	NX::ThrowIfFailed(g_pSwapChain->Present1(0, 0, &parameters));
}

void App::ReleaseUnusedTextures()
{
	NXResourceManager::GetInstance()->GetTextureManager()->ReleaseUnusedTextures();
}

void App::Release()
{
	SafeRelease(m_pRenderer);
	NXResourceManager::GetInstance()->Release();
	SafeRelease(g_dxResources);
}