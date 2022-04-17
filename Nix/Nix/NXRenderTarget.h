#pragma once
#include "NXObject.h"
#include "ShaderStructures.h"

class NXRenderTarget : public NXObject
{
public:
	NXRenderTarget();
	~NXRenderTarget() {}

	void Init();
	void Render();

private:
	void InitVertexIndexBuffer();

private:
	std::vector<VertexPT>		m_vertices;
	std::vector<UINT>			m_indices;
	ComPtr<ID3D11Buffer>		m_pVertexBuffer;
	ComPtr<ID3D11Buffer>		m_pIndexBuffer;
};

