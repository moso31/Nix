#include "NXCodeProcessHelper.h"
#include "NXConvertString.h"
#include "NXTexture.h"
#include "NXGlobalDefinitions.h"

using namespace NXConvert;

std::string NXCodeProcessHelper::RemoveHLSLComment(const std::string& strCode, bool removeUserEditable)
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

	// 如果用户可编辑的注释也要移除，那么这里就可以return了
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
	// 确定kBlocks的起止位置和结束位置
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

		// 将kBlocksPos中的内容替换回strCode
		for (int i = st; i <= ed; ++i)
			result[i] = strCode[i];
	}

	return result;
}

std::string NXCodeProcessHelper::GetFirstEffectiveLine(const std::string& strCode)
{	
	// 先移除注释
	std::string noCommentCode = RemoveHLSLComment(strCode, true);

	// 逐行查找第一有效代码行
	std::istringstream stream(noCommentCode);
	std::string line;
	while (std::getline(stream, line))
	{
		// 去掉前后空白
		size_t first = line.find_first_not_of(" \t\r\n");
		if (first != std::string::npos)
		{
			return line.substr(first); // 返回第一有效代码行
		}
	}

	// 没有找到有效代码行
	return "";
}

int NXCodeProcessHelper::GetLineCount(const std::string& str)
{
	// 统计换行符的数量
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
	std::string strNoCommentCode = RemoveHLSLComment(strCode, false); // 去掉注释

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
		throw std::runtime_error("shader无有效名称");
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
				throw std::runtime_error("括号不匹配");
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
				throw std::runtime_error("括号不匹配");
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
				throw std::runtime_error("括号不匹配");
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
		if (!vals.empty()) // 只要不是空的，就一整行都要
		{
			if (vals[0] == strEndBlock)
			{
				// 去掉最后的换行符，避免每次保存都多一个末尾行
				strCode.pop_back();
				break;
			}

			// title：第一行（函数名和参数）
			if (strTitle.empty())
				strTitle = Trim(str);

			// data：所有内容
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
		// ps: 目前只有1个pass，就是gbuffer，所以nsl文件中的Pass标记，暂时没提供名字，vals.size()长度=1 && vals[0] == "Pass" 就够了.
		// 但长期来看无论是做多pass还是让nsl支持非GBuffer，都需要改良

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
				throw std::runtime_error("括号不匹配");
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
		if (!vals.empty()) // 只要不是空的，就一整行都要
		{
			if (vals[0] == strEndBlock)
			{
				// 去掉最后的换行符，避免每次保存都多一个末尾行
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

std::string NXCodeProcessHelper::BuildHLSL(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, NXMaterialCode& shaderCode, bool bIsGPUTerrain)
{
	// 此处nslPath只有文件名有用，文件内容没用！
	// nslPath在这里唯一作用就是给cbuffer struct生成hash

	int ioLineCounter = 0;

	std::string str;
	str += BuildHLSL_Include(ioLineCounter);
	str += BuildHLSL_Structs(ioLineCounter, oMatData, shaderCode);
	str += BuildHLSL_Params(ioLineCounter, nslPath, oMatData, shaderCode, bIsGPUTerrain);
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

std::string NXCodeProcessHelper::BuildHLSL_Params(int& ioLineCounter, const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode, bool bIsGPUTerrain)
{
	int slot_tex = 0;
	int slot_ss = 0;
	int slot_cb = 0;
	int cb_padding = 0;

	std::string str;
	
	// texture
	for (auto* tex : oMatData.GetTextures())
	{
		if (tex->pTexture->GetResourceType() == NXResourceType::Tex2DArray)
			str += "Texture2DArray ";
		else if (tex->pTexture->GetResourceType() == NXResourceType::TexCube)
			str += "TextureCube ";
		else if (tex->pTexture->GetResourceType() == NXResourceType::Tex2D)
			str += "Texture2D ";
		else
			str += "Texture2D ";

		str += tex->name + " : register(t" + std::to_string(slot_tex++) + ");\n";
	}

	if (bIsGPUTerrain) // GPU Terrain专用
	{
		str += R"(#include "TerrainCommon.fx")"; str += "\n";
		str += "StructuredBuffer<TerrainPatchData> m_patchBuffer : register(t0, space1);\n";
		str += "Texture2DArray m_terrainHeightMap : register(t1, space1);\n";
		str += "Texture2DArray m_terrainSplatMap : register(t2, space1);\n";
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

	bool bIsGBuffer = true; // todo: 扩展其他pass
	if (bIsGBuffer)
	{
		str += "\tfloat shadingModel;\n";
		str += "\tfloat4 customData0;\n"; // 自定义数据，后续可以扩展
	}
	str += "};\n";

	str += "cbuffer " + strMatName + " : register(b" + std::to_string(slot_cb++) + ", space1)\n"; // 2025.5.15 以后规定用户自定义cb参数总是放在space1
	str += "{\n";
	str += "\t" + strMatName + " m;\n"; // 可编辑材质的成员变量约定命名 m。比如 m.albedo, m.metallic
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
#if GPU_INSTANCING
	nointerpolation uint instanceID : TEXCOORD1;
#endif
};

struct PS_OUTPUT
{
	float4 GBufferA : SV_Target0;
	float4 GBufferB : SV_Target1;
	float4 GBufferC : SV_Target2;
	float4 GBufferD : SV_Target3;
};
)";

	ioLineCounter += GetLineCount(str);
	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_PassFuncs(int& ioLineCounter, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	std::string str = R"(
void EncodeGBuffer(NXGBufferParams gBuffer, PS_INPUT input, out PS_OUTPUT Output)
{
	// TODO: 换成VirtPageID，临时代码
	int ix = (int)floor(input.posWS.x);
	int iy = (int)floor(input.posWS.y);
	int iz = (int)floor(input.posWS.z);
	uint ux = (ix % 4096u) & 0xFFFu;
	uint uy = (iy % 4096u) & 0xFFFu;
	uint uz = (iz % 256u) & 0xFFu;
	float packed = input.posWS.y; // (ux << 20) | (uy << 8) | uz;
	Output.GBufferA = packed;

	uint uShadingModel = asuint(m.shadingModel);
	
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
	std::string strVSBegin = R"(PS_INPUT VS(VS_INPUT input
)";

	bool gpuInstancing = g_debug_temporal_enable_terrain_debug;
	if (gpuInstancing)
	{
		strVSBegin += ", uint instanceID : SV_InstanceID\n";
	}

	strVSBegin += R"()
{
	PS_INPUT output;
)";

	if (gpuInstancing)
	{
//		strVSBegin += R"(
//	NXGPUTerrainPatch patch = m_patchBuffer[instanceID];
//)";
		strVSBegin += R"(
	TerrainPatchData patch = m_patchBuffer[instanceID];
)";
	}

	std::string strVSEnd = R"(
#if GPU_INSTANCING
	output.instanceID = instanceID;
#endif
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
	shaderCode.passes[0].vsFunc.hlslLineEnd = ioLineCounter + 1; // TODO: 缕清这里的逻辑，为什么vsEntry/psEntry的hlslLineEnd需要+1，其他地方就不用（现在没时间处理，牵扯的逻辑较多，和CodeEditor、SaveToNSLFile、Extract_()的pop_back()都有关联）

	str += strVSEnd;
	ioLineCounter += GetLineCount(strVSEnd);

	return str;
}

std::string NXCodeProcessHelper::BuildHLSL_Entry_PS(int& ioLineCounter, const NXMaterialData& oMatData, NXMaterialCode& shaderCode)
{
	std::string strPSBegin = R"(
void PS(PS_INPUT input, out PS_OUTPUT Output)
{
	PS_INPUT output;
)";

	bool gpuInstancing = g_debug_temporal_enable_terrain_debug;
	if (gpuInstancing)
	{
//		strPSBegin += R"(
//	uint instanceID = input.instanceID;
//	NXGPUTerrainPatch patch = m_patchBuffer[instanceID];
//)";
		strPSBegin += R"(
	uint instanceID = input.instanceID;
	TerrainPatchData patch = m_patchBuffer[instanceID];
)";
	}

	strPSBegin += R"(
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
