#pragma once
#include "NXInstance.h"
#include "NXShaderDefinitions.h"
#include <memory_resource>

struct NXShaderDataStartPoint
{
	size_t params;
	size_t layout_vs;
	size_t layout_ps;
	size_t code_vs;
	size_t code_ps;
	size_t func;
};

struct NXHLSLGeneratorGBufferStrings
{
	std::string Param;
	std::vector<std::string> Funcs;
	std::vector<std::string> Titles; // TODO��funcs ������������titles��ȥ���������ݺ�ĺ�������ʵ��һ������ �ϲ���
	std::string BodyVS;
	std::string BodyPS;
};

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