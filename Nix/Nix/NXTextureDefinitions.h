#pragma once

enum NXTextureType
{
    TextureType_None,
    TextureType_1D,
    TextureType_2D,
    TextureType_Cube,
    TextureType_2DArray,
    TextureType_3D,
};

enum class NXTextureMode
{
    Raw,			// 使用原生格式
    sRGB,           // sRGB颜色纹理
    Linear,         // 线性颜色纹理
    NormalMap,      // 法线贴图,
    Count
};

struct NXTextureSerializationData
{
    // 纹理类型
    NXTextureMode m_textureType = NXTextureMode::Raw;

    // 是否反转法线Y轴
    bool m_bInvertNormalY = false;

    // 是否生成mipmap
    bool m_bGenerateMipMap = true;

    // 是否是立方体贴图
    bool m_bCubeMap = false;
};
