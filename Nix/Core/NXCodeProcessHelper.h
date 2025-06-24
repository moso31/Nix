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
	// �Ƴ������ı���ע����Ϣ��
	// removeUserEditable: �Ƿ��Ƴ�[BEGIN][END]�����ڵ��û��ɱ༭ע��
	std::string RemoveHLSLComment(const std::string& strCode, bool removeUserEditable = false);
	// ��ȡ��һ����Ч��
	std::string GetFirstEffectiveLine(const std::string& strCode);

	int GetLineCount(const std::string& str);

	bool MoveToNextBranketIn(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& branketName);
	bool MoveToNextBranketOut(std::stack<std::string>& stackBrackets, const std::string& branketName);

	// ��nsl�ļ�����ȡ�����ݺʹ����
	void ExtractShader(const std::string& strCode, NXMaterialData& oMatData, NXMaterialCode& oMatCode);
	void ExtractShader_NXShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialCode& oMatCode);
	void ExtractShader_Params(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialCode& oMatCode);
	void ExtractShader_Params_CBuffer(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialCode& oMatCode);
	void ExtractShader_GlobalFuncs(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialCode& oMatCode);
	void ExtractShader_GlobalFuncBody(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, NXMaterialCode& oMatCode);
	void ExtractShader_SubShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialCode& oMatCode);
	void ExtractShader_SubShader_Pass(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialPassCode& oMatPassCode);
	void ExtractShader_SubShader_Pass_Entry(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, std::string& oStrPassEntryCode);

	// ���������ݺʹ����ת��ΪHLSL����
	std::string BuildHLSL(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, NXMaterialCode& shaderCode, bool bIsGPUTerrain);
	std::string BuildHLSL_Include(int& ioLineCounter);
	std::string BuildHLSL_Params(int& ioLineCounter, const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode, bool bIsGPUTerrain);
	std::string BuildHLSL_GlobalFuncs(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode);
	std::string BuildHLSL_Structs(int& ioLineCounter, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_PassFuncs(int& ioLineCounter, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry_VS(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode);
	std::string BuildHLSL_Entry_PS(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode);

	// ���������ݺʹ����ת��ΪNSL����
	void SaveToNSLFile(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode);
	std::string GenerateNSLParam(const NXMaterialData& data);
}
