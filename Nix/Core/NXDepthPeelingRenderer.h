#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXDepthPeelingRenderer
{
public:
	NXDepthPeelingRenderer(NXScene* pScene);
	~NXDepthPeelingRenderer();

	void Init();
	void Render();

	void Release();

private:
	void RenderLayer(UINT layerIndex, UINT layerCount);
	void CombineLayer(UINT layerIndex, UINT layerCount);

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;
	ComPtr<ID3D11BlendState>			m_pBlendStateOpaque;

	ComPtr<ID3D11SamplerState>			m_pSamplerLinearWrap;
	ComPtr<ID3D11SamplerState>			m_pSamplerLinearClamp;
	ComPtr<ID3D11SamplerState>			m_pSamplerPointClamp;

	NXTexture2D*						m_pSceneDepth[2];
	std::vector<NXTexture2D*>			m_pSceneRT;
	NXTexture2D*						m_pSceneCombineRT;
	NXRenderTarget*						m_pCombineRTData;

	ComPtr<ID3D11VertexShader>			m_pVertexShader2;
	ComPtr<ID3D11PixelShader>			m_pPixelShader2;

	ComPtr<ID3D11VertexShader>			m_pVertexShaderCombine;
	ComPtr<ID3D11PixelShader>			m_pPixelShaderCombine;
	ComPtr<ID3D11InputLayout>			m_pInputLayoutCombine;

	NXScene* m_pScene;

	UINT m_peelingLayerCount;
};
