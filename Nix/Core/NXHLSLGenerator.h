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

struct NXShaderResourceInfo
{
    std::string name;
    NXShaderInputType type;
    UINT registerIndex;
};

using NXShaderResourceInfoArray = std::vector<NXShaderResourceInfo>;

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
	void        ConvertShaderCBufferParam(const size_t hashVal, const std::string& nslCode, std::istringstream& in, std::ostringstream& out);
    std::string ConvertShaderCode(const std::string& nslCode);

private:
    std::string Trim(std::string& str);
};