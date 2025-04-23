#pragma once
#include <string>
#include <sstream>
#include <stack>
#include <vector>
#include <algorithm>

struct NXShaderCodeTexture
{
	std::string type;
	std::string name;
};

struct NXShaderCodeSampler
{
	std::string type;
	std::string name;
};

struct NXShaderCodeValue
{
	std::string type;
	std::string name;
};

struct NXShaderCodeConstantBuffer
{
	std::vector<NXShaderCodeValue> values;
};

struct NXShaderCodeParams
{
	std::vector<NXShaderCodeTexture> textures;
	std::vector<NXShaderCodeSampler> samplers;
	NXShaderCodeConstantBuffer cbuffer;
};

struct NXShaderCodePass
{
	std::string name;
	std::string vsFunc;
	std::string psFunc;
};

struct NXShaderCodeSubShader
{
	std::vector<NXShaderCodePass> passes;
};

struct NXShaderCodeFuncBody
{
	std::string title;
	std::string data;
};

struct NXShaderCodeFunctions
{
	std::vector<NXShaderCodeFuncBody> globalFuncs;
};

struct NXShaderCode
{
	std::string name;
	NXShaderCodeParams params;
	NXShaderCodeFunctions globalFuncs;
	NXShaderCodeSubShader subShader;
};

namespace NXCodeProcessHelper
{
	std::string RemoveHLSLComment(const std::string& strCode);

	bool MoveToNextBranketIn(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& branketName);
	bool MoveToNextBranketOut(std::stack<std::string>& stackBrackets, const std::string& branketName);

	void ExtractShader(const std::string& strCode, NXShaderCode& oShaderData);
	void ExtractShader_NXShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCode& oShaderData);
	void ExtractShader_Params(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCodeParams& oShaderParams);
	void ExtractShader_Params_CBuffer(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCodeConstantBuffer& oShaderCBuffer);
	void ExtractShader_GlobalFuncs(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCodeFunctions& oShaderGlobalFuncs);
	void ExtractShader_GlobalFuncBody(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, NXShaderCodeFuncBody& oShaderFuncBody);
	void ExtractShader_SubShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCodeSubShader& oShaderSubShader);
	void ExtractShader_SubShader_Pass(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCodePass& oShaderSubShaderPass);
	void ExtractShader_SubShader_Pass_EntryPoint(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, std::string& oShaderSubShaderPassEntry);
}
