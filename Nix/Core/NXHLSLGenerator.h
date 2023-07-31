#pragma once
#include "Header.h"
#include "NXInstance.h"
#include "NXShaderDefinitions.h"

class NXHLSLGenerator : public NXInstance<NXHLSLGenerator>
{
public:
	NXHLSLGenerator();
	~NXHLSLGenerator();

	// ��ȡһ�� std::string �ж�����
	int GetLineCount(const std::string& str);

    void EncodeToGBufferShader(const std::string& strHLSLParam, const std::vector<std::string>& strHLSLFuncs, const std::vector<std::string>& strHLSLTitles, const std::string& strHLSLBody, std::string& oHLSLFinal, std::vector<NXHLSLCodeRegion>& oHLSLFuncRegions);
private:

};