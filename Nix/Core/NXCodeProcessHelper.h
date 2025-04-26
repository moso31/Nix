#pragma once
#include <string>
#include <sstream>
#include <filesystem>
#include <stack>
#include <vector>
#include <algorithm>
#include "NXCodeProcessHeader.h"

class NXCustomMaterial;
namespace NXCodeProcessHelper
{
	std::string RemoveHLSLComment(const std::string& strCode);

	bool MoveToNextBranketIn(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& branketName);
	bool MoveToNextBranketOut(std::stack<std::string>& stackBrackets, const std::string& branketName);

	// 从nsl文件中提取出shaderBlock
	void ExtractShader(const std::string& strCode, NXShaderBlock& oShaderData);
	void ExtractShader_NXShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlock& oShaderData);
	void ExtractShader_Params(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlockParams& oShaderParams);
	void ExtractShader_Params_CBuffer(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlockConstantBuffer& oShaderCBuffer);
	void ExtractShader_GlobalFuncs(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlockFunctions& oShaderGlobalFuncs);
	void ExtractShader_GlobalFuncBody(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, NXShaderBlockFuncBody& oShaderFuncBody);
	void ExtractShader_SubShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlockSubShader& oShaderSubShader);
	void ExtractShader_SubShader_Pass(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlockPass& oShaderSubShaderPass);
	void ExtractShader_SubShader_Pass_Entry(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, std::string& oShaderSubShaderPassEntry);

	// 将shaderBlock转换为HLSL代码
	std::string BuildHLSL(const std::filesystem::path& nslPath, const NXShaderBlock& shaderCode, NXCustomMaterial* pMat);
	std::string BuildHLSL_Include(NXCustomMaterial* pMat);
	std::string BuildHLSL_Params(const std::filesystem::path& nslPath, const NXShaderBlock& shaderCode, NXCustomMaterial* pMat);
	std::string BuildHLSL_GlobalFuncs(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat);
	std::string BuildHLSL_Structs(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat);
	std::string BuildHLSL_PassFuncs(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat);
	std::string BuildHLSL_Entry(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat);
	std::string BuildHLSL_Entry_VS(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat);
	std::string BuildHLSL_Entry_PS(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat);

	// 将shaderBlock转换为材质球属性
	void BuildMaterial(const std::filesystem::path& nslPath, const NXShaderBlock& shaderCode, NXMaterialData& oMaterialElement);
}
