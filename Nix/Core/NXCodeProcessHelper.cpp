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

void NXCodeProcessHelper::ExtractShader(const std::string& strCode, NXShaderBlock& oShaderData)
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

void NXCodeProcessHelper::ExtractShader_NXShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlock& oShaderData)
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

void NXCodeProcessHelper::ExtractShader_Params(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlockParams& oShaderParams)
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
				NXShaderBlockTexture newTex;
				newTex.type = vals[0];
				newTex.name = vals[1];
				oShaderParams.textures.push_back(newTex);
			}
			else if (vals[0] == std::string("SamplerState"))
			{
				NXShaderBlockSampler newSampler;
				newSampler.type = vals[0];
				newSampler.name = vals[1];
				oShaderParams.samplers.push_back(newSampler);
			}
		}
	}
}

void NXCodeProcessHelper::ExtractShader_Params_CBuffer(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlockConstantBuffer& oShaderCBuffer)
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
			NXShaderBlockValue newValue;
			newValue.type = vals[0];
			newValue.name = vals[1];
			oShaderCBuffer.values.push_back(newValue);
		}
	}
}

void NXCodeProcessHelper::ExtractShader_GlobalFuncs(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlockFunctions& oShaderGlobalFuncs)
{
	oShaderGlobalFuncs.bodys.clear();

	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.size() == 1)
		{
			if (vals[0] == std::string("[FUNCBEGIN]"))
			{
				ExtractShader_GlobalFuncBody(iss, stackBrackets, "[FUNCEND]", oShaderGlobalFuncs.bodys.emplace_back());
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

void NXCodeProcessHelper::ExtractShader_GlobalFuncBody(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, NXShaderBlockFuncBody& oShaderFuncBody)
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

void NXCodeProcessHelper::ExtractShader_SubShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlockSubShader& oShaderSubShader)
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

void NXCodeProcessHelper::ExtractShader_SubShader_Pass(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXShaderBlockPass& oShaderSubShaderPass)
{
	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.size() == 1)
		{
			if (vals[0] == std::string("[VSBEGIN]"))
			{
				ExtractShader_SubShader_Pass_Entry(iss, stackBrackets, "[VSEND]", oShaderSubShaderPass.vsFunc);
			}
			else if (vals[0] == std::string("[PSBEGIN]"))
			{
				ExtractShader_SubShader_Pass_Entry(iss, stackBrackets, "[PSEND]", oShaderSubShaderPass.psFunc);
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

void NXCodeProcessHelper::ExtractShader_SubShader_Pass_Entry(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, std::string& oShaderSubShaderPassEntry)
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

std::string NXCodeProcessHelper::BuildHLSL(const std::filesystem::path& nslPath, const NXShaderBlock& shaderCode, NXCustomMaterial* pMat)
{
	std::string str;
	str += BuildHLSL_Include(pMat);
	str += BuildHLSL_Params(nslPath, shaderCode, pMat);
	str += BuildHLSL_PassFuncs(shaderCode, pMat);
	str += BuildHLSL_GlobalFuncs(shaderCode, pMat);
	str += BuildHLSL_Structs(shaderCode, pMat);
}

std::string NXCodeProcessHelper::BuildHLSL_Include(NXCustomMaterial* pMat)
{
	std::string str = R"(
#include "Common.fx"
#include "Math.fx"
)";
	return "";
}

std::string NXCodeProcessHelper::BuildHLSL_Params(const std::filesystem::path& nslPath, const NXShaderBlock& shaderCode, NXCustomMaterial* pMat)
{
	int slot_tex = 0;
	int slot_ss = 0;
	int slot_cb = 3;

	std::string strSlotTex = std::to_string(slot_tex);
	std::string strSlotSS = std::to_string(slot_ss);
	std::string strSlotCB = std::to_string(slot_cb);

	std::string str;

	// texture
	const auto& texArr = shaderCode.params.textures;
	for (const auto& tex : texArr)
	{
		str += "Texture2D " + tex.name + " : register(t" + std::to_string(slot_tex) + ");\n";
	}

	// sampler
	const auto& ssArr = shaderCode.params.samplers;
	for (const auto& ss : ssArr)
	{
		str += "SamplerState " + ss.name + " : register(s" + std::to_string(slot_ss) + ");\n";
	}

	// cbuffer
	const auto& cbArr = shaderCode.params.cbuffer;
	std::string strMatName("Mat_" + std::to_string(std::filesystem::hash_value(nslPath)));
	str += "struct " + strMatName + "\n";
	str += "{\n";
	for (const auto& cb : cbArr.values)
	{
		str += "\t" + cb.type + " " + cb.name + ";\n";
	}
	str += "};\n";

	str += "cbuffer " + strMatName + " : register(b" + strSlotCB + ")\n";
	str += "{\n";
	str += "\t" + strMatName + " m;\n"; // 可编辑材质的成员变量约定命名 m。比如 m.albedo, m.metallic
	str += "};\n";

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_GlobalFuncs(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat)
{
	std::string str;
	auto& shaderBodyArr = shaderCode.globalFuncs.bodys;
	for (auto& shaderBody : shaderBodyArr)
	{
		str += shaderBody.data;
		str += "\n";
	}

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Structs(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat)
{
	std::string str = R"(
struct VS_INPUT
{
	float4 pos : POSITION;
	float3 norm : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangent : TANGENT;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float4 posOS : POSITION0;
	float4 posWS : POSITION1;
	float4 posVS : POSITION2;
	float3 normVS : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangentVS : TANGENT;
};

struct PS_OUTPUT
{
	float4 GBufferA : SV_Target0;
	float4 GBufferB : SV_Target1;
	float4 GBufferC : SV_Target2;
	float4 GBufferD : SV_Target3;
};
)";

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_PassFuncs(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat)
{
	std::string str = R"(
void EncodeGBuffer(NXGBufferParams gBuffer, PS_INPUT input, out PS_OUTPUT Output)
{
	uint uShadingModel = asuint(m.shadingModel);

	Output.GBufferA = float4(1.0f, 1.0f, 1.0f, 1.0f);
	
	float3 normalVS = TangentSpaceToViewSpace(gBuffer.normal, input.normVS, input.tangentVS);
	if (uShadingModel == 2) // burley SSS
	{
		Output.GBufferB = float4(normalVS, m.customData0.x);
		Output.GBufferC = float4(gBuffer.albedo, 1.0f);
		Output.GBufferD = float4(gBuffer.roughness, gBuffer.metallic, gBuffer.ao, (float)uShadingModel / 255.0f);
	}
	else if (uShadingModel == 1)
	{
		Output.GBufferB = float4(1.0f, 1.0f, 1.0f, 1.0f);
		Output.GBufferC = float4(gBuffer.albedo, 1.0f);
		Output.GBufferD = float4(1.0f, 1.0f, 1.0f, (float)uShadingModel / 255.0f);
	}
	else 
	{
		Output.GBufferB = float4(normalVS, 1.0f);
		Output.GBufferC = float4(gBuffer.albedo, 1.0f);
		Output.GBufferD = float4(gBuffer.roughness, gBuffer.metallic, gBuffer.ao, (float)uShadingModel / 255.0f);
	}
}
)";
	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Entry(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat)
{
	std::string str;
	str += BuildHLSL_Entry_VS(shaderCode, pMat);
	str += BuildHLSL_Entry_PS(shaderCode, pMat);
	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Entry_VS(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat)
{
	std::string strVSBegin = R"(PS_INPUT VS(VS_INPUT input)
{)";
	std::string strVSEnd = R"(
	return output;
})";

	std::string str;
	str += strVSBegin;
	str += shaderCode.subShader.passes[0].vsFunc + "\n";
	str += strVSEnd;

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Entry_PS(const NXShaderBlock& shaderCode, NXCustomMaterial* pMat)
{
	std::string strPSBegin = R"(
void PS(PS_INPUT input, out PS_OUTPUT Output)
{
    NXGBufferParams o;
	o.albedo = 1.0f.xxx;
	o.normal = 1.0f.xxx;
	o.metallic = 1.0f;
	o.roughness = 1.0f;
	o.ao = 1.0f;
)";

	std::string strPSEnd = R"(
    EncodeGBuffer(o, input, Output);
}
)";

	std::string str;
	str += strPSBegin;
	str += shaderCode.subShader.passes[0].psFunc + "\n";
	str += strPSEnd;

	return str;
}
