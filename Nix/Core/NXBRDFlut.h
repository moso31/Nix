#pragma once
#include "Header.h"

class NXBRDFLut
{
public:
	NXBRDFLut();
	~NXBRDFLut() {}

	void GenerateBRDFLUT();
	void Release();

	ID3D11ShaderResourceView* GetSRV();

private:
	NXTexture2D* m_pTexBRDFLUT;
};
