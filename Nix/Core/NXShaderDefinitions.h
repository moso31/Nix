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
    Float,
    Float2,
    Float3,
    Float4,
    Float4x4,
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
    NXTexture2D* pTexture;
    UINT slotIndex;
};

struct NXMaterialSamplerInfo
{
    ComPtr<ID3D11SamplerState> pSampler;
    UINT slotIndex;
};
