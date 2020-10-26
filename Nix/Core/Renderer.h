#pragma once
#include "Header.h"

class Renderer
{
public:
	void Init();
	void InitRenderer();
	void UpdateSceneData();
	void DrawScene();
	void Release();

private:
	void DrawPrimitives();
	void DrawCubeMap();
	void DrawShadowMap();

private:
	ID3D11InputLayout*			m_pInputLayoutP;
	ID3D11InputLayout*			m_pInputLayoutPNT;

	ID3D11VertexShader*			m_pVertexShader;
	ID3D11VertexShader*			m_pVertexShaderCubeMap;
	ID3D11VertexShader*			m_pVertexShaderOffScreen;
	ID3D11VertexShader*			m_pVertexShaderShadowMap;

	ID3D11PixelShader*			m_pPixelShader;
	ID3D11PixelShader*			m_pPixelShaderCubeMap;
	ID3D11PixelShader*			m_pPixelShaderOffScreen;
	ID3D11PixelShader*			m_pPixelShaderShadowMap;

	ID3D11SamplerState*			m_pSamplerLinearWrap;
	ID3D11SamplerState*			m_pSamplerLinearClamp;
	ID3D11SamplerState*			m_pSamplerShadowMapPCF;	// shadowMap PCF滤波采样，模糊阴影边缘

	std::shared_ptr<NXRenderTarget>	m_renderTarget;

	std::shared_ptr<NXGlobalBufferManager>		m_globalBufferManager;
	std::shared_ptr<NXScene>					m_scene;
	std::shared_ptr<NXPassShadowMap>			m_pPassShadowMap; 
};