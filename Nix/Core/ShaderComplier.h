#pragma once
#include "Header.h"

class ShaderComplier
{
public:
	static HRESULT Compile(std::wstring szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
};