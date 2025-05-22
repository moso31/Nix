#include "NXCodeProcessHelper.h"
#include "NXConvertString.h"
#include "NXTexture.h"

using namespace NXConvert;

std::string NXCodeProcessHelper::RemoveHLSLComment(const std::string& strCode, bool removeUserEditable)
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

	// ����û��ɱ༭��ע��ҲҪ�Ƴ�����ô����Ϳ���return��
	if (removeUserEditable)
		return result;

	struct UserBlock { const char* start; const char* end; };
	static const UserBlock kBlocks[] =
	{
		{"[FUNCBEGIN]", "[FUNCEND]"},
		{"[VSBEGIN]",   "[VSEND]"},
		{"[PSBEGIN]",   "[PSEND]"},
	};

	std::vector<std::pair<int, int>> kBlocksPos;
	// ȷ��kBlocks����ֹλ�úͽ���λ��
	for (const auto& block : kBlocks)
	{
		size_t left = result.find(block.start, 0);
		if (left != std::string::npos)
		{
			size_t right = result.find(block.end, left);
			if (right != std::string::npos)
			{
				kBlocksPos.push_back({ (int)left, (int)right });
			}
		}
	}

	for (const auto& block : kBlocksPos)
	{
		int st = block.first;
		int ed = block.second;

		// ��kBlocksPos�е������滻��strCode
		for (int i = st; i <= ed; ++i)
			result[i] = strCode[i];
	}

	return result;
}

std::string NXCodeProcessHelper::GetFirstEffectiveLine(const std::string& strCode)
{	
	// ���Ƴ�ע��
	std::string noCommentCode = RemoveHLSLComment(strCode, true);

	// ���в��ҵ�һ��Ч������
	std::istringstream stream(noCommentCode);
	std::string line;
	while (std::getline(stream, line))
	{
		// ȥ��ǰ��հ�
		size_t first = line.find_first_not_of(" \t\r\n");
		if (first != std::string::npos)
		{
			return line.substr(first); // ���ص�һ��Ч������
		}
	}

	// û���ҵ���Ч������
	return "";
}

int NXCodeProcessHelper::GetLineCount(const std::string& str)
{
	// ͳ�ƻ��з�������
	int count = 0;
	for (char c : str)
	{
		if (c == '\n')
			count++;
	}
	return count;
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

void NXCodeProcessHelper::ExtractShader(const std::string& strCode, NXMaterialData& oMatData, NXMaterialCode& oMatCode)
{
	std::string strNoCommentCode = RemoveHLSLComment(strCode, false); // ȥ��ע��

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

	if (!shaderCheck || !nameCheck)
		throw std::runtime_error("shader����Ч����");
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
				tx->name = vals[1];
				oMatData.AddTexture(tx);
			}
			else if (vals[0] == std::string("SamplerState"))
			{
				NXMatDataSampler* ss = new NXMatDataSampler();
				ss->name = vals[1];
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
				{
					return;
				}
				throw std::runtime_error("���Ų�ƥ��");
			}
		}
		else if (vals.size() == 2)
		{
			NXMatDataCBuffer* cb = new NXMatDataCBuffer();
			cb->name = vals[1];
			oMatData.AddCBuffer(cb);

			// 5 = length of "float"
			{
				std::string subStr = vals[0].substr(5, vals[0].size() - 5);
				if (!subStr.empty() && subStr[0] >= '0' && subStr[0] <= '9')
				{
					int ofs = subStr[0] - '0';
					if (ofs > 0 && ofs <= 4)
					{
						cb->size = ofs;
					}
				}
			}
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
			{
				// ȥ�����Ļ��з�������ÿ�α��涼��һ��ĩβ��
				strCode.pop_back();
				break;
			}

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

	NXMaterialCodeBlock block;
	block.data = strCode;

	oMatCode.commonFuncs.title.push_back(strTitle);
	oMatCode.commonFuncs.data.push_back(block);
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
				ExtractShader_SubShader_Pass_Entry(iss, stackBrackets, "[VSEND]", oMatPassCode.vsFunc.data);
			}
			else if (vals[0] == std::string("[PSBEGIN]"))
			{
				ExtractShader_SubShader_Pass_Entry(iss, stackBrackets, "[PSEND]", oMatPassCode.psFunc.data);
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
			if (vals[0] == strEndBlock)
			{
				// ȥ�����Ļ��з�������ÿ�α��涼��һ��ĩβ��
				oStrPassEntryCode.pop_back();
				return;
			}

			oStrPassEntryCode += str + "\n";
		}
		else
		{
			oStrPassEntryCode += "\n";
		}
	}
}

std::string NXCodeProcessHelper::BuildHLSL(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, NXMaterialCode& shaderCode)
{
	// �˴�nslPathֻ���ļ������ã��ļ�����û�ã�
	// nslPath������Ψһ���þ��Ǹ�cbuffer struct����hash

	int ioLineCounter = 0;

	std::string str;
	str += BuildHLSL_Include(ioLineCounter);
	str += BuildHLSL_Structs(ioLineCounter, oMatData, shaderCode);
	str += BuildHLSL_Params(ioLineCounter, nslPath, oMatData, shaderCode);
	str += BuildHLSL_PassFuncs(ioLineCounter, oMatData, shaderCode);
	str += BuildHLSL_GlobalFuncs(ioLineCounter, oMatData, shaderCode);
	str += BuildHLSL_Entry(ioLineCounter, oMatData, shaderCode);

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Include(int& ioLineCounter)
{
	std::string str = R"(#include "Common.fx"
#include "Math.fx"
)";
	ioLineCounter += GetLineCount(str);
	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Params(int& ioLineCounter, const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	int slot_tex = 0;
	int slot_ss = 0;
	int slot_cb = 0;
	int cb_padding = 0;

	std::string str;
	
	// texture
	for (auto* tex : oMatData.GetTextures())
	{
		if (tex->pTexture->GetTextureType() == NXTextureType::TextureType_2DArray)
			str += "Texture2DArray ";
		else if (tex->pTexture->GetTextureType() == NXTextureType::TextureType_Cube)
			str += "TextureCube ";
		else if (tex->pTexture->GetTextureType() == NXTextureType::TextureType_2D)
			str += "Texture2D ";
		else
			str += "Texture2D ";

		str += tex->name + " : register(t" + std::to_string(slot_tex++) + ");\n";
	}

	// sampler
	for (auto* ss : oMatData.GetSamplers())
	{
		str += "SamplerState " + ss->name + " : register(s" + std::to_string(slot_ss++) + ");\n";
	}

	// cbuffer
	int padSize = 0;
	std::string strMatName("Mat_" + std::to_string(std::filesystem::hash_value(nslPath)));
	str += "struct " + strMatName + "\n";
	str += "{\n";
	for (auto* cb : oMatData.GetCBuffers())
	{
		std::string strFloat = cb->size > 1 ? std::to_string(cb->size) : "";
		str += "\tfloat" + strFloat + " " + cb->name + ";\n";

		if (padSize + cb->size > 4) padSize = cb->size;
		else padSize = (padSize + cb->size) % 4;
	}

	if (padSize != 0)
	{
		str += "\tfloat" + std::to_string(padSize) + " _padding" + std::to_string(cb_padding++) + ";\n";
	}

	bool bIsGBuffer = true; // todo: ��չ����pass
	if (bIsGBuffer)
	{
		str += "\tfloat shadingModel;\n";
		str += "\tfloat4 customData0;\n"; // �Զ������ݣ�����������չ
	}
	str += "};\n";

	str += "cbuffer " + strMatName + " : register(b" + std::to_string(slot_cb++) + ", space1)\n"; // 2025.5.15 �Ժ�涨�û��Զ���������Ƿ���space1
	str += "{\n";
	str += "\t" + strMatName + " m;\n"; // �ɱ༭���ʵĳ�Ա����Լ������ m������ m.albedo, m.metallic
	str += "};\n";

	ioLineCounter += GetLineCount(str);
	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_GlobalFuncs(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode)
{
	std::string str;
	for (auto& shaderBody : shaderCode.commonFuncs.data)
	{
		shaderBody.hlslLineBegin = ioLineCounter + 1;
		std::string strFunc = shaderBody.data + "\n";
		ioLineCounter += GetLineCount(strFunc);
		shaderBody.hlslLineEnd = ioLineCounter + 1;

		str += strFunc;
	}

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Structs(int& ioLineCounter, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	std::string str = R"(
struct VS_INPUT
{
	float4 pos : POSITION;
	float3 norm : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangent : TANGENT;
#ifdef GPU_INSTANCING
	float4 row0 : GPUINSWORLD0;
	float4 row1 : GPUINSWORLD1;
	float4 row2 : GPUINSWORLD2;
	float4 row3 : GPUINSWORLD3;
#endif
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

#include "Instancing.fx"
)";

	ioLineCounter += GetLineCount(str);
	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_PassFuncs(int& ioLineCounter, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
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
	ioLineCounter += GetLineCount(str);
	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Entry(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode)
{
	std::string str;
	str += BuildHLSL_Entry_VS(ioLineCounter, oMatData, shaderCode);
	str += BuildHLSL_Entry_PS(ioLineCounter, oMatData, shaderCode);

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Entry_VS(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode)
{
	std::string strVSBegin = R"(PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output;
)";
	std::string strVSEnd = R"(
	return output;
}
)";

	std::string str;
	str += strVSBegin;
	ioLineCounter += GetLineCount(strVSBegin);

	shaderCode.passes[0].vsFunc.hlslLineBegin = ioLineCounter + 1; 
	std::string strVSFunc = shaderCode.passes[0].vsFunc.data;
	str += strVSFunc;
	ioLineCounter += GetLineCount(strVSFunc);
	shaderCode.passes[0].vsFunc.hlslLineEnd = ioLineCounter + 1; // TODO: ����������߼���ΪʲôvsEntry/psEntry��hlslLineEnd��Ҫ+1�������ط��Ͳ��ã�����ûʱ�䴦��ǣ�����߼��϶࣬��CodeEditor��SaveToNSLFile��Extract_()��pop_back()���й�����

	str += strVSEnd;
	ioLineCounter += GetLineCount(strVSEnd);

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Entry_PS(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode)
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
	ioLineCounter += GetLineCount(strPSBegin);

	shaderCode.passes[0].psFunc.hlslLineBegin = ioLineCounter + 1;
	std::string strPSFunc = shaderCode.passes[0].psFunc.data;
	str += strPSFunc;
	ioLineCounter += GetLineCount(strPSFunc);
	shaderCode.passes[0].psFunc.hlslLineEnd = ioLineCounter + 1;

	str += strPSEnd;
	ioLineCounter += GetLineCount(strPSEnd);

	return str;
}

void NXCodeProcessHelper::SaveToNSLFile(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	std::string str;
	str += "NXShader " + shaderCode.shaderName + "\n";
	str += "{\n";

	str += GenerateNSLParam(oMatData);

	str += "\tGlobalFuncs\n";
	str += "\t{\n";

	for (size_t i = 0; i < shaderCode.commonFuncs.title.size(); ++i)
	{
		str += "[FUNCBEGIN]\n";
		str += shaderCode.commonFuncs.data[i].data;
		str += "\n";
		str += "[FUNCEND]\n";
	}

	str += "\t}\n"; // GlobalFuncs

	str += "\tSubShader\n";
	str += "\t{\n";

	for (auto& pass : shaderCode.passes)
	{
		str += "\t\tPass\n";
		str += "\t\t{\n";

		str += "[VSBEGIN]\n";
		str += pass.vsFunc.data;
		str += "\n"; 
		str += "[VSEND]\n";

		str += "\n";

		str += "[PSBEGIN]\n";
		str += pass.psFunc.data;
		str += "\n";
		str += "[PSEND]\n";

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

std::string NXCodeProcessHelper::GenerateNSLParam(const NXMaterialData& matData)
{
	std::string result;
	result += "\tParams\n";
	result += "\t{\n";

	for (auto* tx : matData.GetTextures())
	{
		result += "\t\t";
		result += "Tex2D ";
		result += tx->name;
		result += "\n";
	}

	for (auto* ss : matData.GetSamplers())
	{
		result += "\t\t";
		result += "SamplerState ";
		result += ss->name;
		result += "\n";
	}

	result += "\t\tCBuffer\n";
	result += "\t\t{\n";
	for (auto* cb : matData.GetCBuffers())
	{
		result += "\t\t\t";
		result += "float";
		result += cb->size > 1 ? std::to_string(cb->size) : "";
		result += " ";
		result += cb->name;
		result += "\n";
	}

	result += "\t\t}\n"; // CBuffer
	result += "\t}\n"; // Params

	return result;
}
