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

	// ��MSE/GUI��������nsl
	std::string GenerateNSL(const NXMaterialData& data);

	// ��nsl�ļ�����ȡ�����ݺʹ����
	void ExtractShader(const std::string& strCode, NXMaterialData& oMatData, NXMaterialDataIntermediate& oMatIntermediate, NXMaterialCode& oMatCode);
	void ExtractShader_NXShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialDataIntermediate& oMatIntermediate, NXMaterialCode& oMatCode);
	void ExtractShader_Params(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialDataIntermediate& oMatIntermediate, NXMaterialCode& oMatCode);
	void ExtractShader_Params_CBuffer(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialDataIntermediate& oMatIntermediate, NXMaterialCode& oMatCode);
	void ExtractShader_GlobalFuncs(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialCode& oMatCode);
	void ExtractShader_GlobalFuncBody(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, NXMaterialCode& oMatCode);
	void ExtractShader_SubShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialCode& oMatCode);
	void ExtractShader_SubShader_Pass(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialPassCode& oMatPassCode);
	void ExtractShader_SubShader_Pass_Entry(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, std::string& oStrPassEntryCode);

	// ���������ݺʹ����ת��ΪHLSL����
	std::string BuildHLSL(const std::filesystem::path& nslPath, NXMaterialData& oMatData, const NXMaterialDataIntermediate oMatDataIntermediate, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Include();
	std::string BuildHLSL_Params(const std::filesystem::path& nslPath, NXMaterialData& oMatData, const NXMaterialDataIntermediate oMatDataIntermediate, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_GlobalFuncs(NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Structs(NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_PassFuncs(NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry(NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry_VS(NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry_PS(NXMaterialData& oMatData, const NXMaterialCode& shaderCode);

	// ���������ݺʹ����ת��ΪHLSL����
	void SaveToNSLFile(const std::filesystem::path& nslPath, NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
}
