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
    NXCBufferInputType type;
    std::variant<float, Vector2, Vector3, Vector4, Matrix> data;
};

struct NXCBufferInfo
{
    std::vector<NXCBufferElem> elems;
    UINT cbSlotIndex;
};

using NXCBufferInfoArray = std::unordered_map<std::string, NXCBufferInfo>;

struct NXShaderResourceInfo
{
    NXShaderInputType type;
    UINT registerIndex;
    NXCBufferInfoArray cbInfos;
};

using NXShaderResourceInfoArray = std::unordered_map<std::string, NXShaderResourceInfo>;
