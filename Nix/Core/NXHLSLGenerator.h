#pragma once
#include "Header.h"
#include "NXInstance.h"

enum NXShaderInputType
{
    Unknown,
    CBuffer,
    Texture,
    Sampler,
};

enum NXCBufferInputType
{
	Float,
	Float2,
	Float3,
	Float4,
	Float4x4,
};

struct NXCBufferInfo
{
    NXCBufferInputType type;
    std::variant<float, Vector2, Vector3, Vector4, Matrix> data;
};

struct NXShaderResourceInfo
{
    NXShaderInputType type;
    UINT registerIndex;
    NXCBufferInfoArray cbInfos;
};

using NXShaderResourceInfoArray = std::unordered_map<std::string, NXShaderResourceInfo>;
using NXCBufferInfoArray = std::unordered_map<std::string, NXCBufferInfo>;

class NXHLSLGenerator : public NXInstance<NXHLSLGenerator>
{
public:
	NXHLSLGenerator();
	~NXHLSLGenerator();

    bool LoadShaderFromFile(const std::filesystem::path& shaderPath, std::string& shaderContent);
    void ExtractShaderData(const std::string& shader, std::string& nslParams, std::string& nslCode);
    bool ConvertShaderToHLSL(const std::filesystem::path& shaderPath, const std::string& nslCode, std::string& oHLSLCodeHead, std::string& oHLSLCodeBody, NXShaderResourceInfoArray& oSRInfoArray);
    void EncodeToGBufferShader(const std::string& strHLSLParam, const std::string& strHLSLBody, std::string& oHLSLFinal);

private:
	std::string ConvertShaderParam(const std::filesystem::path& shaderPath, const std::string& nslCode, NXShaderResourceInfoArray& oSRInfoArray);
	void        ConvertShaderCBufferParam(const size_t hashVal, const std::string& nslCode, std::istringstream& in, std::ostringstream& out, NXCBufferInfoArray& oCBInfoArray);
    std::string ConvertShaderCode(const std::string& nslCode);

private:
    std::string Trim(std::string& str);
};