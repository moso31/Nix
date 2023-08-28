#pragma once
#include "Header.h"
#include "Ntr.h"

class NXBRDFLut
{
public:
	NXBRDFLut();
	~NXBRDFLut() {}

	void GenerateBRDFLUT();
	void Release();

	ID3D11ShaderResourceView* GetSRV();

private:
	Ntr<NXTexture2D> m_pTexBRDFLUT;
};
