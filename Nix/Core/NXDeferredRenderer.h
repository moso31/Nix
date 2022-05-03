#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXDeferredRenderer
{
public:
	NXDeferredRenderer(NXScene* pScene);
	~NXDeferredRenderer();

	void Init();
	void RenderGBuffer();
	void Render();

	void Release();

private:
	void InitVertexIndexBuffer();

private:
	std::vector<VertexPT>				m_vertices;
	std::vector<UINT>					m_indices;

	ComPtr<ID3D11InputLayout>			m_pInputLayoutGBuffer;
	ComPtr<ID3D11InputLayout>			m_pInputLayoutRender;

	ComPtr<ID3D11Buffer>				m_pVertexBuffer;
	ComPtr<ID3D11Buffer>				m_pIndexBuffer;

	// 渲染RT0-RT4时使用此组Shader
	ComPtr<ID3D11VertexShader>			m_pVertexShader; 
	ComPtr<ID3D11PixelShader>			m_pPixelShader;

	// 进行最终渲染时使用此组Shaders
	ComPtr<ID3D11VertexShader>			m_pVertexShaderRender; 
	ComPtr<ID3D11PixelShader>			m_pPixelShaderRender;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilStateGBuffer;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerStateGBuffer;
	ComPtr<ID3D11BlendState>			m_pBlendStateGBuffer;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilStateLighting;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerStateLighting;
	ComPtr<ID3D11BlendState>			m_pBlendStateLighting;

	ComPtr<ID3D11SamplerState>			m_pSamplerLinearWrap;
	ComPtr<ID3D11SamplerState>			m_pSamplerLinearClamp;

	NXScene* m_pScene;
};
