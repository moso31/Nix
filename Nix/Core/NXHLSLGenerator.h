#pragma once
#include "Header.h"
#include "NXInstance.h"
#include "NXShaderDefinitions.h"

class NXHLSLGenerator : public NXInstance<NXHLSLGenerator>
{
public:
	NXHLSLGenerator();
	~NXHLSLGenerator();

    void EncodeToGBufferShader(const std::string& strHLSLParam, const std::string& strHLSLFuncs, const std::string& strHLSLBody, std::string& oHLSLFinal);
};