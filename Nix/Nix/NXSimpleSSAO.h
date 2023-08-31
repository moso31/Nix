#pragma once
#include "BaseDefs/DX11.h"
#include "BaseDefs/Math.h"

class NXTexture2D;
class NXSimpleSSAO
{
	struct ConstantBufferSSAOParams
	{
		ConstantBufferSSAOParams() : radius(1.0f), bias(0.15f), directLightingStrength(0.0f) {}
		float radius;
		float bias;
		float directLightingStrength;
		float _0;
	};

public:
	NXSimpleSSAO();
	~NXSimpleSSAO();

	void Init();
	void OnResize(const Vector2& rtSize);
	void Update();
	void Render(ID3D11ShaderResourceView* pSRVNormal, ID3D11ShaderResourceView* pSRVPosition, ID3D11ShaderResourceView* pSRVDepthPrepass);

	ID3D11ShaderResourceView* GetSRV();

	float GetRadius() { return m_ssaoParams.radius; }
	void SetRadius(float radius) { m_ssaoParams.radius = radius; }

	float GetBias() { return m_ssaoParams.bias; }
	void SetBias(float bias) { m_ssaoParams.bias = bias; }

	float GetDirectLightingStrength() { return m_ssaoParams.directLightingStrength; }
	void SetDirectLightingStrength(float strength) { m_ssaoParams.directLightingStrength = strength; }

	void Release();

private:
	void InitSSAOParams();
	void GenerateSamplePosition();

private:
	ComPtr<ID3D11ComputeShader>			m_pComputeShader;

	NXTexture2D* m_pTexSSAO;

	std::vector<Vector4>		m_samplePosition;
	ComPtr<ID3D11Buffer> 		m_pCBSamplePositions;

	ConstantBufferSSAOParams	m_ssaoParams;
	ComPtr<ID3D11Buffer>		m_pCBSSAOParams;

	Vector2 m_rtSize;
};
