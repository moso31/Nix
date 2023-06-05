#include "NXHLSLGenerator.h"

NXHLSLGenerator::NXHLSLGenerator()
{
}

NXHLSLGenerator::~NXHLSLGenerator()
{
}

void NXHLSLGenerator::EncodeToGBufferShader(const std::string& strHLSLParam, const std::string& strHLSLFuncs, const std::string& strHLSLBody, std::string& oHLSLFinal)
{
    auto strInclude = R"(#include "Common.fx"
#include "Math.fx"
)";

	auto strIOStruct = R"(struct VS_INPUT
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
)";

	auto strPSBegin = R"(void PS(PS_INPUT input, out PS_OUTPUT Output)
{
)";
    auto strPSEnd = R"(
}
)";

    oHLSLFinal = strInclude + strHLSLParam + strHLSLFuncs + strIOStruct + 
		strPSBegin + strHLSLBody + strPSEnd;
}
