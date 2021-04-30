#pragma once
#include "Header.h"

class NXSimpleSSAO
{
public:
	NXSimpleSSAO();
	~NXSimpleSSAO();

	void Init(const Vector2& AOBufferSize);
	void Render(ID3D11ShaderResourceView* pSRVNormal, ID3D11ShaderResourceView* pSRVDepthPrepass);

private:
	void GenerateSamplePosition();

private:
	ComPtr<ID3D11ComputeShader>			m_pComputeShader;

	ComPtr<ID3D11ShaderResourceView>	m_pSRVSSAO;
	ComPtr<ID3D11UnorderedAccessView>	m_pUAVSSAO;

	//std::vector<Vector3>	m_samplePosition;
	//ComPtr<ID3D11Buffer> 	m_pCBSamplePositions;
};
