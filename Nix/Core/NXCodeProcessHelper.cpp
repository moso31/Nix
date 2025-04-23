#include "NXCodeProcessHelper.h"
#include "NXConvertString.h"

using namespace NXConvert;

std::string NXCodeProcessHelper::RemoveHLSLComment(const std::string& strCode)
{
	// 移除一个strCode中的所有注释内容（格式必须是HLSL）。
	// 规则：
	// 1. 从上往下遍历
	// 2. 若先检测到 "//"
	//		2a. 去掉当前行//之后的所有内容（result 相同位置字符 全换成空格）
	// 3. 若先检测到 "/*"
	//		3a. 向后遍历，直到寻找到"*/"停下
	//		3b. 去掉/*...*/之间的所有内容（result 相同位置字符 全换成空格）

	std::string result = strCode;

	size_t i = 0;
	size_t end = strCode.length();
	while (i < end) // 1. 从上往下遍历
	{
		size_t pos2 = strCode.find("//", i);
		size_t pos3 = strCode.find("/*", i);

		if (pos2 < pos3) // 2. 若先检测到 "//"
		{
			size_t pos2a = strCode.find("\n", pos2);
			if (pos2a == std::string::npos) pos2a = end;

			// 2a. 去掉当前行//之后的所有内容（result 相同位置字符 全换成空格）
			result.replace(pos2, pos2a - pos2, pos2a - pos2, ' ');
			i = pos2a; // 继续向下遍历
		}
		else if (pos3 < pos2) // 3. 若先检测到 "/*"
		{
			// 3a. 向后遍历，直到寻找到"*/"停下
			size_t pos3a = strCode.find("*/", pos3);
			if (pos3a == std::string::npos) pos3a = end;

			// 3b. 去掉/*...*/之间的所有内容（result 相同位置字符 全换成空格）
			result.replace(pos3, pos3a - pos3 + 2, pos3a - pos3 + 2, ' ');
			i = pos3a + 2; // 继续向下遍历
		}
		else 
		{
			// 没有注释了，直接退出
			break;
		}
	}

	return result;
}

bool NXCodeProcessHelper::MoveToNextBranketIn(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& branketName)
{
	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.empty())
			continue;

		if (vals.size() > 0 && vals[0] == "{")
		{
			stackBrackets.push(branketName);
			return true;
		}
	}

	return false;
}

bool NXCodeProcessHelper::MoveToNextBranketOut(std::stack<std::string>& stackBrackets, const std::string& branketName)
{
	if (stackBrackets.top() == branketName)
	{
		stackBrackets.pop();
		return true;
	}
	return false;
}

void NXCodeProcessHelper::ExtractShader(const std::string& strCode, NXShaderCode& oShaderData)
{
	std::string strNoCommentCode = RemoveHLSLComment(strCode); // 去掉注释

	std::istringstream iss(strNoCommentCode);
	std::string str;
	std::stack<std::string> stackBrackets;

	bool shaderCheck = false;
	bool nameCheck = false;

	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.size() > 0)
		{
			if (vals[0].c_str() == std::string("NXShader")) shaderCheck = true;
		}

		if (vals.size() > 1)
		{
			if (vals[1].c_str()) nameCheck = true;
		}

		if (shaderCheck && nameCheck)
		{
			oShaderData.name = vals[1];
			MoveToNextBranketIn(iss, stackBrackets, "NXShader");
			ExtractShader_NXShader(iss, stackBrackets, oShaderData);
			break;
		}
	}
}

void NXCodeProcessHelper::ExtractShader_NXShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCode& oShaderData)
{
	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);

		if (vals.size() == 1)
		{
			if (vals[0] == std::string("Params"))
			{
				MoveToNextBranketIn(iss, stackBrackets, "Params");
				ExtractShader_Params(iss, stackBrackets, oShaderData.params);
			}
			else if (vals[0] == std::string("GlobalFuncs"))
			{
				MoveToNextBranketIn(iss, stackBrackets, "GlobalFuncs");
				ExtractShader_GlobalFuncs(iss, stackBrackets, oShaderData.globalFuncs);
			}
			else if (vals[0] == std::string("SubShader"))
			{
				MoveToNextBranketIn(iss, stackBrackets, "SubShader");
				ExtractShader_SubShader(iss, stackBrackets, oShaderData.subShader);
			}
		}
	}
}

void NXCodeProcessHelper::ExtractShader_Params(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCodeParams& oShaderParams)
{
	oShaderParams.textures.clear();
	oShaderParams.samplers.clear();

	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.size() == 1)
		{
			if (vals[0] == std::string("CBuffer"))
			{
				MoveToNextBranketIn(iss, stackBrackets, "CBuffer");
				ExtractShader_Params_CBuffer(iss, stackBrackets, oShaderParams.cbuffer);
			}
			if (vals[0] == std::string("}"))
			{
				if (MoveToNextBranketOut(stackBrackets, "Params")) 
					return;
				throw std::runtime_error("括号不匹配");
			}
		}
		else if (vals.size() == 2)
		{
			if (vals[0] == std::string("Tex2D"))
			{
				NXShaderCodeTexture newTex;
				newTex.type = vals[0];
				newTex.name = vals[1];
				oShaderParams.textures.push_back(newTex);
			}
			else if (vals[0] == std::string("SamplerState"))
			{
				NXShaderCodeSampler newSampler;
				newSampler.type = vals[0];
				newSampler.name = vals[1];
				oShaderParams.samplers.push_back(newSampler);
			}
		}
	}
}

void NXCodeProcessHelper::ExtractShader_Params_CBuffer(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCodeConstantBuffer& oShaderCBuffer)
{
	oShaderCBuffer.values.clear();

	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.size() == 1)
		{
			if (vals[0] == std::string("}"))
			{
				if (MoveToNextBranketOut(stackBrackets, "CBuffer"))
					return;
				throw std::runtime_error("括号不匹配");
			}
		}
		else if (vals.size() == 2)
		{
			NXShaderCodeValue newValue;
			newValue.type = vals[0];
			newValue.name = vals[1];
			oShaderCBuffer.values.push_back(newValue);
		}
	}
}

void NXCodeProcessHelper::ExtractShader_GlobalFuncs(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCodeFunctions& oShaderGlobalFuncs)
{
	oShaderGlobalFuncs.globalFuncs.clear();

	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.size() == 1)
		{
			if (vals[0] == std::string("[FUNCBEGIN]"))
			{
				ExtractShader_GlobalFuncBody(iss, stackBrackets, "[FUNCEND]", oShaderGlobalFuncs.globalFuncs.emplace_back());
			}
			if (vals[0] == std::string("}"))
			{
				if (MoveToNextBranketOut(stackBrackets, "GlobalFuncs"))
					return;
				throw std::runtime_error("括号不匹配");
			}
		}
	}
}

void NXCodeProcessHelper::ExtractShader_GlobalFuncBody(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, NXShaderCodeFuncBody& oShaderFuncBody)
{
	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (!vals.empty()) // 只要不是空的，就一整行都要
		{
			if (vals[0] == strEndBlock) return;

			// title：第一行（函数名和参数）
			if (oShaderFuncBody.title.empty())
				oShaderFuncBody.title = Trim(str);

			// data：所有内容
			oShaderFuncBody.data += str + "\n";
		}
		else
		{
			oShaderFuncBody.data += "\n";
		}
	}
}

void NXCodeProcessHelper::ExtractShader_SubShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCodeSubShader& oShaderSubShader)
{
	oShaderSubShader.passes.clear();

	std::string str;
	while (std::getline(iss, str))
	{
		// ps: 目前只有1个pass，就是gbuffer，所以nsl文件中的Pass标记，暂时没提供名字，vals.size()长度=1 && vals[0] == "Pass" 就够了.
		// 但长期来看无论是做多pass还是让nsl支持非GBuffer，都需要改良

		auto& vals = split(str);
		if (vals.size() == 1) 
		{
			if (vals[0] == std::string("Pass"))
			{
				MoveToNextBranketIn(iss, stackBrackets, "Pass");
				ExtractShader_SubShader_Pass(iss, stackBrackets, oShaderSubShader.passes.emplace_back());
			}
		}
	}
}

void NXCodeProcessHelper::ExtractShader_SubShader_Pass(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderCodePass& oShaderSubShaderPass)
{
	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.size() == 1)
		{
			if (vals[0] == std::string("[VSBEGIN]"))
			{
				ExtractShader_SubShader_Pass_EntryPoint(iss, stackBrackets, "[VSEND]", oShaderSubShaderPass.vsFunc);
			}
			else if (vals[0] == std::string("[PSBEGIN]"))
			{
				ExtractShader_SubShader_Pass_EntryPoint(iss, stackBrackets, "[PSEND]", oShaderSubShaderPass.psFunc);
			}
			else if (vals[0] == std::string("}"))
			{
				if (MoveToNextBranketOut(stackBrackets, "Pass"))
					return;
				throw std::runtime_error("括号不匹配");
			}
		}
	}
}

void NXCodeProcessHelper::ExtractShader_SubShader_Pass_EntryPoint(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, std::string& oShaderSubShaderPassEntry)
{
	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (!vals.empty()) // 只要不是空的，就一整行都要
		{
			if (vals[0] == strEndBlock) return;

			oShaderSubShaderPassEntry += str + "\n";
		}
		else
		{
			oShaderSubShaderPassEntry += "\n";
		}
	}
}
