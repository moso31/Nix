#include "NXCodeProcessHelper.h"
#include "NXConvertString.h"
#include "NXTexture.h"

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

void NXCodeProcessHelper::SetDefaultCBufferGUIParam(NXGUIStyle_CBufferItem& guiStyle)
{
	switch (guiStyle.style)
	{
	case NXMSE_CBufferStyle::Value:
	case NXMSE_CBufferStyle::Value2:
	case NXMSE_CBufferStyle::Value3:
	case NXMSE_CBufferStyle::Value4:
	default:
		guiStyle.guiParams0 = 0.01f; // speed
		guiStyle.guiParams1 = 0.0f;  // --- (unused)

	case NXMSE_CBufferStyle::Slider:
	case NXMSE_CBufferStyle::Slider2:
	case NXMSE_CBufferStyle::Slider3:
	case NXMSE_CBufferStyle::Slider4:
	case NXMSE_CBufferStyle::Color3:
	case NXMSE_CBufferStyle::Color4:
		guiStyle.guiParams0 = 0.0f; // min
		guiStyle.guiParams1 = 1.0f; // max
	}
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

std::string NXCodeProcessHelper::GenerateNSL(const NXMSEPackDatas& guiDatas)
{
	std::string result;
	result += "NXShader\n";
	result += "{\n";

	result += "\tParams\n;";
	result += "\t{\n";

	std::vector<NXMSE_CBufferData*> cbArr;
	std::vector<NXMSE_TextureData*> txArr;
	std::vector<NXMSE_SamplerData*> ssArr;

	for (auto* data : guiDatas.datas)
	{
		switch (data->pMaterialData->GetType())
		{
		case NXMaterialBaseType::CBuffer: cbArr.push_back(static_cast<NXMSE_CBufferData*>(data)); break;
		case NXMaterialBaseType::Texture: txArr.push_back(static_cast<NXMSE_TextureData*>(data)); break;
		case NXMaterialBaseType::Sampler: ssArr.push_back(static_cast<NXMSE_SamplerData*>(data)); break;
		default: break;
		}
	}

	for (auto* tx : txArr)
	{
		result += "\t\t";
		result += "Tex2D ";
		result += tx->MaterialData()->name;
		result += ";\n";
	}

	for (auto* ss : ssArr)
	{
		result += "\t\t";
		result += "SamplerState";
		result += ss->MaterialData()->name;
		result += ";\n";
	}

	result += "CBuffer\n";
	result += "\t\t{\n";
	for (auto* cb : cbArr)
	{
		result += "\t\t\t";
		result += "float" + std::to_string(cb->MaterialData()->size);
		result += " ";
		result += cb->MaterialData()->name;
		result += ";\n";
	}

	result += "\t\t}\n"; // CBuffer
	result += "\t}\n"; // Params
	result += "}\n";
}

void NXCodeProcessHelper::ExtractShader(const std::string& strCode, NXMaterialData& oMatData, NXMSEPackDatas& oGUIData, NXMaterialCode& oMatCode)
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
			oMatCode.shaderName = vals[1];
			MoveToNextBranketIn(iss, stackBrackets, "NXShader");
			ExtractShader_NXShader(iss, stackBrackets, oMatData, oGUIData, oMatCode);
			break;
		}
	}
}

void NXCodeProcessHelper::ExtractShader_NXShader(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMSEPackDatas& oGUIData, NXMaterialCode& oMatCode)
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
				ExtractShader_Params(iss, stackBrackets, oMatData, oGUIData, oMatCode);
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

void NXCodeProcessHelper::ExtractShader_Params(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMSEPackDatas& oGUIData, NXMaterialCode& oMatCode)
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
				ExtractShader_Params_CBuffer(iss, stackBrackets, oMatData, oGUIData, oMatCode);
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
				NXMaterialData_Texture* newTex = new NXMaterialData_Texture(vals[1]);
				oMatData.datas.push_back(newTex);

				NXMSE_TextureData* newTexGUI = new NXMSE_TextureData();
				newTexGUI->pMaterialLink = newTex;
				oGUIData.datas.push_back(newTexGUI);
			}
			else if (vals[0] == std::string("SamplerState"))
			{
				NXMaterialData_Sampler* newSampler = new NXMaterialData_Sampler(vals[1]);
				oMatData.datas.push_back(newSampler);

				NXMSE_SamplerData* newSamplerGUI = new NXMSE_SamplerData();
				newSamplerGUI->pMaterialLink = newSampler;
				oGUIData.datas.push_back(newSamplerGUI);
			}
		}
	}
}

void NXCodeProcessHelper::ExtractShader_Params_CBuffer(std::istringstream& iss, std::stack<std::string>& stackBrackets, NXMaterialData& oMatData, NXMSEPackDatas& oGUIData, NXMaterialCode& oMatCode)
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
				throw std::runtime_error("括号不匹配");
			}
		}
		else if (vals.size() == 2)
		{
			NXMaterialData_CBuffer* newCBuffer = new NXMaterialData_CBuffer(vals[1]);
			oMatData.datas.push_back(newCBuffer);

			NXMSE_CBufferData* newCBufferGUI = new NXMSE_CBufferData();
			newCBufferGUI->pMaterialLink = newCBuffer;
			oGUIData.datas.push_back(newCBufferGUI);
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
				break;

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

	oMatCode.commonFuncs.title.push_back(strTitle);
	oMatCode.commonFuncs.data.push_back(strCode);
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
	// 注意这个方法不会用nslPath记录的内容构建HLSL；而是使用oMatData+shaderCode。
	// nslPath在这里唯一作用就是给cbuffer struct生成hash

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

	std::vector<NXMaterialData_CBuffer*> cbArr;
	std::vector<NXMaterialData_Texture*> txArr;
	std::vector<NXMaterialData_Sampler*> ssArr;

	for (auto* data : oMatData.datas)
	{
		switch (data->GetType())
		{
		case NXMaterialBaseType::CBuffer: cbArr.push_back(static_cast<NXMaterialData_CBuffer*>(data)); break;
		case NXMaterialBaseType::Texture: txArr.push_back(static_cast<NXMaterialData_Texture*>(data)); break;
		case NXMaterialBaseType::Sampler: ssArr.push_back(static_cast<NXMaterialData_Sampler*>(data)); break;
		default: break;
		}
	}

	// texture
	for (auto* tex : txArr)
	{
		str += "Texture2D " + tex->name + " : register(t" + std::to_string(slot_tex) + ");\n";
	}

	// sampler
	for (auto* ss : ssArr)
	{
		str += "SamplerState " + ss->name + " : register(s" + std::to_string(slot_ss) + ");\n";
	}

	// cbuffer
	std::string strMatName("Mat_" + std::to_string(std::filesystem::hash_value(nslPath)));
	str += "struct " + strMatName + "\n";
	str += "{\n";
	for (auto* cb : cbArr)
	{
		str += "\tfloat" + std::to_string(cb->size) + " " + cb->name + ";\n";
	}
	bool bIsGBuffer = true; // todo: 扩展其他pass
	if (bIsGBuffer)
	{
		str += "\tfloat shadingModel;\n";
		str += "\tfloat4 customData0;\n"; // 自定义数据，后续可以扩展
	}
	str += "};\n";

	str += "cbuffer " + strMatName + " : register(b" + strSlotCB + ")\n";
	str += "{\n";
	str += "\t" + strMatName + " m;\n"; // 可编辑材质的成员变量约定命名 m。比如 m.albedo, m.metallic
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

NXMaterialData NXCodeProcessHelper::BuildMaterialData(const NXMSEPackDatas& guiData)
{
	NXMaterialData result;
	for (auto* gui : guiData.datas)
	{
		auto type = gui->pMaterialData->GetType();
		if (type == NXMaterialBaseType::CBuffer)
		{
			NXMSE_CBufferData* guiCB = (NXMSE_CBufferData*)gui;

			NXMaterialData_CBuffer* newCBuffer = new NXMaterialData_CBuffer(gui->pMaterialData->name);
			newCBuffer->data = guiCB->MaterialData()->data;
			newCBuffer->size = guiCB->MaterialData()->size;
			result.datas.push_back(newCBuffer);
		}
		else if (type == NXMaterialBaseType::Texture)
		{
			NXMSE_TextureData* guiTex = (NXMSE_TextureData*)gui;

			NXMaterialData_Texture* newTexture = new NXMaterialData_Texture(gui->pMaterialData->name);
			newTexture->pTexture = guiTex->MaterialData()->pTexture;
			result.datas.push_back(newTexture);
		}
		else if (type == NXMaterialBaseType::Sampler)
		{
			NXMSE_SamplerData* guiSampler = (NXMSE_SamplerData*)gui;

			NXMaterialData_Sampler* newSampler = new NXMaterialData_Sampler(gui->pMaterialData->name);
			newSampler->filter = guiSampler->MaterialData()->filter;
			newSampler->addressU = guiSampler->MaterialData()->addressU;
			newSampler->addressV = guiSampler->MaterialData()->addressV;
			newSampler->addressW = guiSampler->MaterialData()->addressW;
			result.datas.push_back(newSampler);
		}
	}

	result.sets.shadingModel = guiData.settings.shadingModel;
	return result;
}

void NXCodeProcessHelper::SaveToNSLFile(const std::filesystem::path& nslPath, const NXMaterialData& oMatData, const NXMaterialCode& shaderCode)
{
	std::vector<NXMaterialData_CBuffer*> cbArr;
	std::vector<NXMaterialData_Texture*> txArr;
	std::vector<NXMaterialData_Sampler*> ssArr;

	for (auto* data : oMatData.datas)
	{
		switch (data->GetType())
		{
		case NXMaterialBaseType::CBuffer: cbArr.push_back(static_cast<NXMaterialData_CBuffer*>(data)); break;
		case NXMaterialBaseType::Texture: txArr.push_back(static_cast<NXMaterialData_Texture*>(data)); break;
		case NXMaterialBaseType::Sampler: ssArr.push_back(static_cast<NXMaterialData_Sampler*>(data)); break;
		default: break;
		}
	}


	std::string str;
	str += "NXShader \"" + shaderCode.shaderName + "\"\n";
	str += "\n";
	str += "\tParams\n";
	str += "\t{\n";
	
	for (auto* tx : txArr)
	{
		str += "\t\t";
		str += "Tex2D ";
		str += tx->name;
		str += "\n";
	}

	for (auto* ss : ssArr)
	{
		str += "\t\t";
		str += "SamplerState ";
		str += ss->name;
		str += "\n";
	}

	str += "\t\tCBuffer\n";
	str += "\t\t{\n";
	for (auto* cb : cbArr)
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
