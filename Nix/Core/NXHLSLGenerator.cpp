#include "NXHLSLGenerator.h"

NXHLSLGenerator::NXHLSLGenerator()
{
}

NXHLSLGenerator::~NXHLSLGenerator()
{
}

int NXHLSLGenerator::GetLineCount(const std::string& str) 
{
	return (int)std::count(str.begin(), str.end(), '\n') + 1;
}

void NXHLSLGenerator::EncodeToGBufferShader(const NXHLSLGeneratorGBufferStrings& strBlock, std::string& oHLSLFinal, std::vector<NXHLSLCodeRegion>& oHLSLFuncRegions)
{
	std::string strInclude = R"(#include "Common.fx"
#include "Math.fx"
)";

	std::string strIOStruct = R"(
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

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posOS = input.pos;
	output.posWS = mul(input.pos, m_world);
	output.posVS = mul(output.posWS, m_view);
	output.posSS = mul(output.posVS, m_projection);
	output.normVS = normalize(mul(input.norm, (float3x3)m_worldViewInverseTranspose));
	output.tex = input.tex;
	output.tangentVS = normalize(mul(input.tangent, (float3x3)m_worldViewInverseTranspose));
	return output;
}

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

	std::string strGBufferOutType = "GBuffer_StandardLit";
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

	oHLSLFinal = strInclude + strBlock.Param;

	// 将子函数提前声明，避免子函数顺序不对导致编译失败
	for (int i = 1; i < strBlock.Titles.size(); i++)
		oHLSLFinal = oHLSLFinal + strBlock.Titles[i] + ";\n";

	// 处理子函数并记录行列号
	int lineCount = GetLineCount(oHLSLFinal);
	oHLSLFuncRegions.resize(strBlock.Funcs.size() + 1);
	for (int i = 0; i < strBlock.Funcs.size(); i++)
	{
		auto& strHLSLFunc = strBlock.Funcs[i];
		int lineCountFunc = GetLineCount(strHLSLFunc) - 1;
		oHLSLFinal = oHLSLFinal + strHLSLFunc;
		oHLSLFuncRegions[i + 1] = { lineCount, lineCount + lineCountFunc - 1 };
		lineCount += lineCountFunc;
	}

	oHLSLFinal = oHLSLFinal + strIOStruct + strPSBegin;

	// 处理PS主函数并记录行列号
	lineCount = GetLineCount(oHLSLFinal);
	int mainFuncLineCount = GetLineCount(strBlock.BodyPS);
	oHLSLFuncRegions[0] = { lineCount - 2, lineCount + mainFuncLineCount };
	lineCount += mainFuncLineCount;
	oHLSLFinal += strBlock.BodyPS + strPSEnd;
}
