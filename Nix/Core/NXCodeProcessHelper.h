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
	std::string BuildHLSL(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Include();
	std::string BuildHLSL_Params(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_GlobalFuncs(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Structs(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_PassFuncs(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry_VS(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry_PS(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);

	// 将材质数据和代码块转换为NSL代码
	void SaveToNSLFile(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string GenerateNSLParam(const NXMaterialData& data);
}
