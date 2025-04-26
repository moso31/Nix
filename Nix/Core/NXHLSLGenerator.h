#pragma once
#include "NXInstance.h"
#include "NXShaderDefinitions.h"
#include <memory_resource>

class NXHLSLGenerator : public NXInstance<NXHLSLGenerator>
{
public:
	NXHLSLGenerator();
	virtual ~NXHLSLGenerator();

	// ��ȡһ�� std::string �ж�����
	int GetLineCount(const std::string& str);

    void EncodeToGBufferShader(const NXHLSLGeneratorGBufferStrings& strBlock, std::string& oHLSLFinal, std::vector<NXHLSLCodeRegion>& oHLSLFuncRegions);
private:

};