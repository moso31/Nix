#pragma once

enum NXShaderInputType
{
    Unknown,
    CBuffer,
    Texture,
    Sampler,
};

enum NXCBufferInputType
{
    Float = 1,
    Float2 = 2,
    Float3 = 3,
    Float4 = 4,
};

struct NXCBufferElem
{
    std::string name;
    NXCBufferInputType type;
    int memoryIndex;
};

struct NXMaterialCBufferInfo
{
    std::vector<NXCBufferElem> elems;
    UINT slotIndex;
};

struct NXMaterialTextureInfo
{
    std::string name;
    NXTexture2D* pTexture;
    UINT slotIndex;
};

struct NXMaterialSamplerInfo
{
    std::string name;
    ComPtr<ID3D11SamplerState> pSampler;
    UINT slotIndex;
};
