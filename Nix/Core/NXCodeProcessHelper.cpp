#include "NXCodeProcessHelper.h"
#include "NXConvertString.h"
#include "NXTexture.h"

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

std::string NXCodeProcessHelper::GenerateNSL(const NXMaterialData& matData)
{
	std::string result;
	result += "NXShader\n";
	result += "{\n";

	result += "\tParams\n;";
	result += "\t{\n";

	for (auto* tx : matData.GetTextures())
	{
		result += "\t\t";
		result += "Tex2D ";
		result += tx->name;
		result += ";\n";
	}

	for (auto* ss : matData.GetSamplers())
	{
		result += "\t\t";
		result += "SamplerState";
		result += ss->name;
		result += ";\n";
	}

	result += "CBuffer\n";
	result += "\t\t{\n";
	for (auto* cb : matData.GetCBuffers())
	{
		result += "\t\t\t";
		result += "float" + std::to_string(cb->size);
		result += " ";
		result += cb->name;
		result += ";\n";
	}

	result += "\t\t}\n"; // CBuffer
	result += "\t}\n"; // Params
	result += "}\n";
}

void NXCodeProcessHelper::ExtractShader(const std::string& strCode, NXMaterialData& oMatData, NXMaterialCode& oMatCode)
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
			oMatCode.shaderName = vals[1];
			MoveToNextBranketIn(iss, stackBrackets, "NXShader");
			ExtractShader_NXShader(iss, stackBrackets, oMatData, oMatCode);
			break;
		}
	}
}

void NXCodeProcessHelper::ExtractShader_NXShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialCode& oMatCode)
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
				ExtractShader_Params(iss, stackBrackets, oMatData, oMatCode);
			}
			else if (vals[0] == std::string("GlobalFuncs"))
			{
				MoveToNextBranketIn(iss, stackBrackets, "GlobalFuncs");
				ExtractShader_GlobalFuncs(iss, stackBrackets, oMatCode);
			}
			else if (vals[0] == std::string("SubShader"))
			{
				MoveToNextBranketIn(iss, stackBrackets, "SubShader");
				ExtractShader_SubShader(iss, stackBrackets, oMatCode);
			}
		}
	}
}

void NXCodeProcessHelper::ExtractShader_Params(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialCode& oMatCode)
{
	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.size() == 1)
		{
			if (vals[0] == std::string("CBuffer"))
			{
				MoveToNextBranketIn(iss, stackBrackets, "CBuffer");
				ExtractShader_Params_CBuffer(iss, stackBrackets, oMatData, oMatCode);
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
				NXMatDataTexture* tx = new NXMatDataTexture();
				oMatData.AddTexture(tx);
			}
			else if (vals[0] == std::string("SamplerState"))
			{
				NXMatDataSampler* ss = new NXMatDataSampler();
				oMatData.AddSampler(ss);
			}
		}
	}
}

void NXCodeProcessHelper::ExtractShader_Params_CBuffer(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMaterialCode& oMatCode)
{
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
			NXMatDataCBuffer* cb = new NXMatDataCBuffer();
			oMatData.AddCBuffer(cb);
		}
	}
}

void NXCodeProcessHelper::ExtractShader_GlobalFuncs(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialCode& oMatCode)
{
	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.size() == 1)
		{
			if (vals[0] == std::string("[FUNCBEGIN]"))
			{
				ExtractShader_GlobalFuncBody(iss, stackBrackets, "[FUNCEND]", oMatCode);
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

void NXCodeProcessHelper::ExtractShader_GlobalFuncBody(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, NXMaterialCode& oMatCode)
{
	std::string str;
	std::string strTitle;
	std::string strCode;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (!vals.empty()) // ֻҪ���ǿյģ���һ���ж�Ҫ
		{
			if (vals[0] == strEndBlock)
				break;

			// title����һ�У��������Ͳ�����
			if (strTitle.empty())
				strTitle = Trim(str);

			// data����������
			strCode += str + "\n";
		}
		else
		{
			strCode += "\n";
		}
	}

	oMatCode.commonFuncs.title.push_back(strTitle);
	oMatCode.commonFuncs.data.push_back(strCode);
}

void NXCodeProcessHelper::ExtractShader_SubShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialCode& oMatCode)
{
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
				ExtractShader_SubShader_Pass(iss, stackBrackets, oMatCode.passes.emplace_back());
			}
		}
	}
}

void NXCodeProcessHelper::ExtractShader_SubShader_Pass(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialPassCode& oMatPassCode)
{
	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (vals.size() == 1)
		{
			if (vals[0] == std::string("[VSBEGIN]"))
			{
				ExtractShader_SubShader_Pass_Entry(iss, stackBrackets, "[VSEND]", oMatPassCode.vsFunc);
			}
			else if (vals[0] == std::string("[PSBEGIN]"))
			{
				ExtractShader_SubShader_Pass_Entry(iss, stackBrackets, "[PSEND]", oMatPassCode.psFunc);
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

void NXCodeProcessHelper::ExtractShader_SubShader_Pass_Entry(std::istringstream& iss, std::stack<std::string>& stackBrackets, const std::string& strEndBlock, std::string& oStrPassEntryCode)
{
	std::string str;
	while (std::getline(iss, str))
	{
		auto& vals = split(str);
		if (!vals.empty()) // ֻҪ���ǿյģ���һ���ж�Ҫ
		{
			if (vals[0] == strEndBlock) return;

			oStrPassEntryCode += str + "\n";
		}
		else
		{
			oStrPassEntryCode += "\n";
		}
	}
}

std::string NXCodeProcessHelper::BuildHLSL(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	// ע���������������nslPath��¼�����ݹ���HLSL������ʹ��oMatData+shaderCode��
	// nslPath������Ψһ���þ��Ǹ�cbuffer struct����hash

	std::string str;
	str += BuildHLSL_Include();
	str += BuildHLSL_Params(nslPath, oMatData, shaderCode);
	str += BuildHLSL_PassFuncs(oMatData, shaderCode);
	str += BuildHLSL_GlobalFuncs(oMatData, shaderCode);
	str += BuildHLSL_Structs(oMatData, shaderCode);
}

std::string NXCodeProcessHelper::BuildHLSL_Include()
{
	std::string str = R"(
#include "Common.fx"
#include "Math.fx"
)";
	return "";
}

std::string NXCodeProcessHelper::BuildHLSL_Params(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	int slot_tex = 0;
	int slot_ss = 0;
	int slot_cb = 3;

	std::string strSlotTex = std::to_string(slot_tex);
	std::string strSlotSS = std::to_string(slot_ss);
	std::string strSlotCB = std::to_string(slot_cb);

	std::string str;
	
	// texture
	for (auto* tex : oMatData.GetTextures())
	{
		str += "Texture2D " + tex->name + " : register(t" + std::to_string(slot_tex) + ");\n";
	}

	// sampler
	for (auto* ss : oMatData.GetSamplers())
	{
		str += "SamplerState " + ss->name + " : register(s" + std::to_string(slot_ss) + ");\n";
	}

	// cbuffer
	std::string strMatName("Mat_" + std::to_string(std::filesystem::hash_value(nslPath)));
	str += "struct " + strMatName + "\n";
	str += "{\n";
	for (auto* cb : oMatData.GetCBuffers())
	{
		str += "\tfloat" + std::to_string(cb->size) + " " + cb->name + ";\n";
	}
	bool bIsGBuffer = true; // todo: ��չ����pass
	if (bIsGBuffer)
	{
		str += "\tfloat shadingModel;\n";
		str += "\tfloat4 customData0;\n"; // �Զ������ݣ�����������չ
	}
	str += "};\n";

	str += "cbuffer " + strMatName + " : register(b" + strSlotCB + ")\n";
	str += "{\n";
	str += "\t" + strMatName + " m;\n"; // �ɱ༭���ʵĳ�Ա����Լ������ m������ m.albedo, m.metallic
	str += "};\n";

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_GlobalFuncs(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	std::string str;
	for (auto& shaderBody : shaderCode.commonFuncs.data)
	{
		str += shaderBody;
		str += "\n";
	}

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Structs(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
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

std::string NXCodeProcessHelper::BuildHLSL_PassFuncs(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
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

std::string NXCodeProcessHelper::BuildHLSL_Entry(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	std::string str;
	str += BuildHLSL_Entry_VS(oMatData, shaderCode);
	str += BuildHLSL_Entry_PS(oMatData, shaderCode);
	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Entry_VS(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	std::string strVSBegin = R"(PS_INPUT VS(VS_INPUT input)
{)";
	std::string strVSEnd = R"(
	return output;
})";

	std::string str;
	str += strVSBegin;
	str += shaderCode.passes[0].vsFunc + "\n";
	str += strVSEnd;

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Entry_PS(const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
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
	str += shaderCode.passes[0].psFunc + "\n";
	str += strPSEnd;

	return str;
}

void NXCodeProcessHelper::SaveToNSLFile(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	std::string str;
	str += "NXShader \"" + shaderCode.shaderName + "\"\n";
	str += "\n";
	str += "\tParams\n";
	str += "\t{\n";
	
	for (auto* tx : oMatData.GetTextures())
	{
		str += "\t\t";
		str += "Tex2D ";
		str += tx->name;
		str += "\n";
	}

	for (auto* ss : oMatData.GetSamplers())
	{
		str += "\t\t";
		str += "SamplerState ";
		str += ss->name;
		str += "\n";
	}

	str += "\t\tCBuffer\n";
	str += "\t\t{\n";
	for (auto* cb : oMatData.GetCBuffers())
	{
		str += "\t\t\t";
		str += "float" + std::to_string(cb->size);
		str += " ";
		str += cb->name;
		str += "\n";
	}

	str += "\t\t}\n"; // CBuffer
	str += "\t}\n"; // Params

	str += "\tGlobalFuncs\n";
	str += "\t{\n";

	for (size_t i = 0; i < shaderCode.commonFuncs.title.size(); ++i)
	{
		str += "\t\t[FUNCBEGIN]\n";
		str += shaderCode.commonFuncs.data[i];
		str += "\t\t[FUNCEND]\n";
	}

	str += "\t}\n"; // GlobalFuncs

	str += "\tSubShader\n";
	str += "\t{\n";

	for (auto& pass : shaderCode.passes)
	{
		str += "\t\tPass\n";
		str += "\t\t{\n";

		str += "\t\t\t[VSBEGIN]\n";
		str += pass.vsFunc;
		str += "\t\t\t[VSEND]\n";

		str += "\t\t\t[PSBEGIN]\n";
		str += pass.psFunc;
		str += "\t\t\t[PSEND]\n";

		str += "\t\t}\n"; // Pass
	}

	str += "\t}\n"; // SubShader
	str += "}\n"; // NXShader

	// save to file
	std::ofstream file(nslPath);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + nslPath.string());
	}
	file << str;
	file.close();
}
