#include "NXCodeProcessHelper.h"
#include "NXConvertString.h"

using namespace NXConvert;

std::string NXCodeProcessHelper::RemoveHLSLComment(const std::string& strCode)
{
	// �Ƴ�һ��strCode�е�����ע�����ݣ���ʽ������HLSL����
	// ����
	// 1. �������±���
	// 2. ���ȼ�⵽ "//"
	//		2a. ȥ����ǰ��//֮����������ݣ�result ��ͬλ���ַ� ȫ���ɿո�
	// 3. ���ȼ�⵽ "/*"
	//		3a. ��������ֱ��Ѱ�ҵ�"*/"ͣ��
	//		3b. ȥ��/*...*/֮����������ݣ�result ��ͬλ���ַ� ȫ���ɿո�

	std::string result = strCode;

	size_t i = 0;
	size_t end = strCode.length();
	while (i < end) // 1. �������±���
	{
		size_t pos2 = strCode.find("//", i);
		size_t pos3 = strCode.find("/*", i);

		if (pos2 < pos3) // 2. ���ȼ�⵽ "//"
		{
			size_t pos2a = strCode.find("\n", pos2);
			if (pos2a == std::string::npos) pos2a = end;

			// 2a. ȥ����ǰ��//֮����������ݣ�result ��ͬλ���ַ� ȫ���ɿո�
			result.replace(pos2, pos2a - pos2, pos2a - pos2, ' ');
			i = pos2a; // �������±���
		}
		else if (pos3 < pos2) // 3. ���ȼ�⵽ "/*"
		{
			// 3a. ��������ֱ��Ѱ�ҵ�"*/"ͣ��
			size_t pos3a = strCode.find("*/", pos3);
			if (pos3a == std::string::npos) pos3a = end;

			// 3b. ȥ��/*...*/֮����������ݣ�result ��ͬλ���ַ� ȫ���ɿո�
			result.replace(pos3, pos3a - pos3 + 2, pos3a - pos3 + 2, ' ');
			i = pos3a + 2; // �������±���
		}
		else 
		{
			// û��ע���ˣ�ֱ���˳�
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
	std::string strNoCommentCode = RemoveHLSLComment(strCode); // ȥ��ע��

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
				throw std::runtime_error("���Ų�ƥ��");
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
				throw std::runtime_error("���Ų�ƥ��");
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
				throw std::runtime_error("���Ų�ƥ��");
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
		if (!vals.empty()) // ֻҪ���ǿյģ���һ���ж�Ҫ
		{
			if (vals[0] == strEndBlock) return;

			// title����һ�У��������Ͳ�����
			if (oShaderFuncBody.title.empty())
				oShaderFuncBody.title = Trim(str);

			// data����������
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
		// ps: Ŀǰֻ��1��pass������gbuffer������nsl�ļ��е�Pass��ǣ���ʱû�ṩ���֣�vals.size()����=1 && vals[0] == "Pass" �͹���.
		// ��������������������pass������nsl֧�ַ�GBuffer������Ҫ����

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
				throw std::runtime_error("���Ų�ƥ��");
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
		if (!vals.empty()) // ֻҪ���ǿյģ���һ���ж�Ҫ
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
