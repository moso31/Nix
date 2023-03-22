#pragma once
#include "header.h"
#include "ShaderStructures.h"

struct CBufferDepthPeelingParams
{
	int depthLayer;
	Vector3 _0;
};

class NXDepthPeelingRenderer
{
public:
	NXDepthPeelingRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut);
	~NXDepthPeelingRenderer();

	void Init();
	void Render();

	void Release();

private:
	void InitConstantBuffer();
	void RenderLayer();

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerStateFront;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerStateBack;
	ComPtr<ID3D11BlendState>			m_pBlendState;
	ComPtr<ID3D11BlendState>			m_pBlendStateOpaque;

	ComPtr<ID3D11SamplerState>			m_pSamplerLinearWrap;
	ComPtr<ID3D11SamplerState>			m_pSamplerLinearClamp;
	ComPtr<ID3D11SamplerState>			m_pSamplerPointClamp;

	NXTexture2D*						m_pSceneDepth[2];
	std::vector<NXTexture2D*>			m_pSceneRT;
	NXTexture2D*						m_pSceneCombineRT;
	NXRenderTarget*						m_pCombineRTData;

	ComPtr<ID3D11VertexShader>			m_pVertexShaderDepthPeeling;
	ComPtr<ID3D11PixelShader>			m_pPixelShaderDepthPeeling;

	ComPtr<ID3D11VertexShader>			m_pVertexShaderCombine;
	ComPtr<ID3D11PixelShader>			m_pPixelShaderCombine;
	ComPtr<ID3D11InputLayout>			m_pInputLayoutCombine;

	ComPtr<ID3D11Buffer>				m_cbDepthPeelingParams;
	CBufferDepthPeelingParams			m_cbDepthPeelingParamsData;

	NXBRDFLut* m_pBRDFLut;
	NXScene* m_pScene;

	UINT m_peelingLayerCount;
};
