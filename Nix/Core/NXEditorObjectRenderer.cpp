#include "NXEditorObjectRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXRenderTarget.h"
#include "NXScene.h"

NXEditorObjectRenderer::NXEditorObjectRenderer() :
	m_pRT(nullptr)
{
}

NXEditorObjectRenderer::~NXEditorObjectRenderer()
{
}

void NXEditorObjectRenderer::Init()
{
	m_pRT = new NXRenderTarget();
	m_pRT->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\Editor.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutEditorObject, ARRAYSIZE(NXGlobalInputLayout::layoutEditorObject), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\Editor.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<false, false, D3D11_COMPARISON_ALWAYS>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<false, false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA>::Create();
}

void NXEditorObjectRenderer::Render()
{
	g_pUDA->BeginEvent(L"Editor objects");

	m_pScene->IsRenderableObject

	g_pUDA->EndEvent();
}

void NXEditorObjectRenderer::Release()
{
}
