#include "NXHLSLGenerator.h"

NXHLSLGenerator::NXHLSLGenerator()
{
}

NXHLSLGenerator::~NXHLSLGenerator()
{
}

bool NXHLSLGenerator::LoadShaderFromFile(const std::filesystem::path& shaderPath, std::string& oShader)
{
    if (!std::filesystem::exists(shaderPath))
    {
        printf("Error: Shader file not found: %s\n", shaderPath.string().c_str());
        return false;
    }

    std::ifstream shaderFile(shaderPath);
    if (!shaderFile.is_open())
    {
        printf("Error: Unable to open shader file: %s\n", shaderPath.string().c_str());
        return false;
    }

    oShader = std::string(std::istreambuf_iterator<char>(shaderFile), std::istreambuf_iterator<char>());
    shaderFile.close();
    return true;
}

void NXHLSLGenerator::ExtractShaderData(const std::string& shader, std::string& nslParams, std::string& nslCode)
{
    // 查找 Params 和 Code 块的开始和结束位置
    const auto paramsStart = shader.find("Params");
    const auto codeStart = shader.find("Code");
    const auto paramsEnd = codeStart - 1;
    const auto codeEnd = shader.size();

    // 提取 Params 和 Code 块
    nslParams = shader.substr(paramsStart, paramsEnd - paramsStart);
    nslCode = shader.substr(codeStart, codeEnd - codeStart);
}

bool NXHLSLGenerator::ConvertShaderToHLSL(const std::filesystem::path& shaderPath, const std::string& shader, std::string& oHLSLCodeHead, std::string& oHLSLCodeBody)
{
    std::string nslParams, nslCode;
    ExtractShaderData(shader, nslParams, nslCode);
    oHLSLCodeHead = ConvertShaderParam(shaderPath, nslParams);
    oHLSLCodeBody = ConvertShaderCode(nslCode);
	return false;
}

void NXHLSLGenerator::EncodeToGBufferShader(const std::string& strHLSLParam, const std::string& strHLSLBody, std::string& oHLSLFinal)
{
    auto strInclude = R"(#include "Common.fx"
#include "Math.fx"
#include "PBRMaterials.fx"

)";

    auto strOther = R"(struct VS_INPUT
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

void PS(PS_INPUT input, out PS_OUTPUT Output)
{
)";
    auto strEnd = R"(
}
)";

    oHLSLFinal = strInclude + strHLSLParam + strOther + strHLSLBody + strEnd;
}

std::string NXHLSLGenerator::ConvertShaderParam(const std::filesystem::path& shaderPath, const std::string& nslParams)
{
    std::map<std::string, std::string> typeToPrefix
    {
        {"Tex2D", "Texture2D"},
        {"SamplerState", "SamplerState"},
        {"CBuffer", "cbuffer"}
    };

    std::map<std::string, char> typeToRegisterPrefix
    {
        {"Tex2D", 't'},
        {"SamplerState", 's'},
        {"CBuffer", 'b'}
    };

    std::map<std::string, int> typeToRegisterIndex
    {
        {"Tex2D", 1},
        {"SamplerState", 0},
        {"CBuffer", 3}
    };

    std::istringstream in(nslParams);
    std::ostringstream out;

    std::string line;

    bool inParam = false;
    bool inParamBrace = false;
    while (std::getline(in, line))
    {
        std::string type, name;
        std::istringstream lineStream(line);
        std::getline(lineStream, type, ':');
        std::getline(lineStream, name, ':');
        type = Trim(type);
        name = Trim(name);

        // 先找Params
        if (type == "Params")
        {
            inParam = true;
            continue;
        }

        if (!inParam) continue;

        // 再找左括号
        if (type == "{")
        {
            inParamBrace = true;
            continue;
        }

        if (!inParamBrace) continue;

        // 进入Param内部

        if (type == "}")
        {
            inParam = false;
            inParamBrace = false;
            continue;
        }

        if (typeToPrefix.find(type) != typeToPrefix.end())
        {
            auto matHashVal = std::filesystem::hash_value(shaderPath);

            if (type == "CBuffer")
            {
                std::ostringstream strMatName;
                strMatName << "Mat_" << matHashVal;

                std::ostringstream strMatStruct;
                strMatStruct << "struct " << strMatName.str();
                strMatStruct << "\n{\n";
                // CBuffer
                ConvertShaderCBufferParam(matHashVal, nslParams, in, strMatStruct);
                strMatStruct << "};\n";

                out << strMatStruct.str();
                out << typeToPrefix[type] << " CBuffer_" << strMatName.str() << " : register(" << typeToRegisterPrefix[type] << typeToRegisterIndex[type]++ << ")";
                out << "\n{\n";
                out << "\t" << strMatName.str() << " " << name << ";\n";
                out << "}\n";
            }
            else
            {
                // 2023.4.9 Texture和SamplerState 之后应该会有WrapMode之类的需求。
                // 暂时还没想好怎么写，先空着
                out << typeToPrefix[type] << " " << name << " : register(" << typeToRegisterPrefix[type] << typeToRegisterIndex[type]++ << ")";
                out << ";\n";
            }
            continue;
        }
    }

    return out.str();
}

void NXHLSLGenerator::ConvertShaderCBufferParam(const size_t hashVal, const std::string& nslCode, std::istringstream& in, std::ostringstream& out)
{
    std::string line;
    bool inParamBrace = false;
    while (std::getline(in, line))
    {
        std::string type, name;
        std::istringstream lineStream(line);
        std::getline(lineStream, type, ':');
        std::getline(lineStream, name, ':');
        type = Trim(type);
        name = Trim(name);

        // 找左括号
        if (type == "{")
        {
            inParamBrace = true;
            continue;
        }

        if (!inParamBrace) continue;

        if (type == "}")
        {
            inParamBrace = false;
            continue;
        }

        // 给 CBuffer 填充变量
        out << "\t" << type << " " << name << ";\n";
        continue;
    }
}

std::string NXHLSLGenerator::ConvertShaderCode(const std::string& nslCode)
{
    std::istringstream in(nslCode);
    std::ostringstream out;

    std::string line;
    
    bool inCode = false;
    bool inCodeBrace = false;

    while (std::getline(in, line))
    {
        std::string type, name;
		std::istringstream lineStream(line);
		std::getline(lineStream, type, ':');
		std::getline(lineStream, name, ':');
		type = Trim(type);
		name = Trim(name);

		// 先找Code
        if (type == "Code")
        {
			inCode = true;
			continue;
		}

		if (!inCode) continue;

		// 再找左括号
        if (type == "{")
        {
			inCodeBrace = true;
			continue;
		}

		if (!inCodeBrace) continue;

		// 进入Code内部
        if (type == "}")
        {
			inCode = false;
			inCodeBrace = false;
			continue;
		}

        out << line << "\n";
    }

    return out.str();
}

std::string NXHLSLGenerator::Trim(std::string& str)
{
    // 去掉 str 中的所有空格和tab
    const auto begin = str.find_first_not_of(" \t");
    if (begin == std::string::npos) return "";
    const auto end = str.find_last_not_of(" \t");
    return str.substr(begin, end - begin + 1);
}
