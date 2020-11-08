#pragma once
#include "NXPrimitive.h"

class NXRenderTarget : public NXPrimitive
{
public:
	NXRenderTarget();
	~NXRenderTarget() {}

	void Init();
	void Render();

	void InitVertexIndexBuffer() override;

private:
	std::vector<VertexPNT>	m_vertices;
};

