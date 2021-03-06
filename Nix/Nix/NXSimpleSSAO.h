#pragma once
#include "Header.h"

class NXSimpleSSAO
{
	struct ConstantBufferSSAOParams
	{
		ConstantBufferSSAOParams() : radius(1.0f) {}
		float radius;
		Vector3 _0;
	};

public:
	NXSimpleSSAO();
	~NXSimpleSSAO();

	void Init(const Vector2& AOBufferSize);
	void Update();
	void Render(ID3D11ShaderResourceView* pSRVNormal, ID3D11ShaderResourceView* pSRVPosition, ID3D11ShaderResourceView* pSRVDepthPrepass);

	ID3D11ShaderResourceView*	GetSRV()	{ return m_pSRVSSAO.Get(); }

	float GetRadius() { return m_ssaoParams.radius; }
	void SetRadius(float radius) { m_ssaoParams.radius = radius; }

private:
	void InitSSAOParams();
	void GenerateSamplePosition();

private:
	ComPtr<ID3D11ComputeShader>			m_pComputeShader;

	ComPtr<ID3D11ShaderResourceView>	m_pSRVSSAO;
	ComPtr<ID3D11UnorderedAccessView>	m_pUAVSSAO;

	std::vector<Vector4>		m_samplePosition;
	ComPtr<ID3D11Buffer> 		m_pCBSamplePositions;

	ConstantBufferSSAOParams	m_ssaoParams;
	ComPtr<ID3D11Buffer>		m_pCBSSAOParams;
};
