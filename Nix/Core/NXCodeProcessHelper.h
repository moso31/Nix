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
	// 移除整个文本的注释信息。
	// removeUserEditable: 是否移除[BEGIN][END]区块内的用户可编辑注释
	std::string RemoveHLSLComment(const std::string& strCode, bool removeUserEditable = false);
	// 获取第一个有效行
	std::string GetFirstEffectiveLine(const std::string& strCode);

	int GetLineCount(const std::string& str);

	bool MoveToNextBranketIn(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& branketName);
	bool MoveToNextBranketOut(std::stack<std::string>& stackBrackets, const std::string& branketName);

	// 从nsl文件中提取出数据和代码块
	void ExtractShader(const std::string& strCode, NXMaterialData& oMatData, NXMaterialCode& oMatCode);
	void ExtractShader_NXShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialCode& oMatCode);
	void ExtractShader_Params(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialCode& oMatCode);
	void ExtractShader_Params_CBuffer(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialCode& oMatCode);
	void ExtractShader_GlobalFuncs(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialCode& oMatCode);
	void ExtractShader_GlobalFuncBody(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, NXMaterialCode& oMatCode);
	void ExtractShader_SubShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialCode& oMatCode);
	void ExtractShader_SubShader_Pass(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialPassCode& oMatPassCode);
	void ExtractShader_SubShader_Pass_Entry(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, std::string& oStrPassEntryCode);

	// 将材质数据和代码块转换为HLSL代码
	std::string BuildHLSL(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, NXMaterialCode& shaderCode, bool bIsGPUTerrain);
	std::string BuildHLSL_Include(int& ioLineCounter);
	std::string BuildHLSL_Params(int& ioLineCounter, const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode, bool bIsGPUTerrain);
	std::string BuildHLSL_GlobalFuncs(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode);
	std::string BuildHLSL_Structs(int& ioLineCounter, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_PassFuncs(int& ioLineCounter, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry_VS(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry_PS(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode);

	// 将材质数据和代码块转换为NSL代码
	void SaveToNSLFile(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string GenerateNSLParam(const NXMaterialData& data);
}
