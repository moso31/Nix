#pragma once

enum class NXResourceType
{
    None,
    Buffer,
    Tex1D,
    Tex2D,
    TexCube,
    Tex2DArray,
    Tex3D,
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
    bool m_bGenerateMipMap = false;

    // 是否是立方体贴图
    bool m_bCubeMap = false;

    // 仅.raw文件使用，记录raw格式的宽高和字节大小
    int m_rawWidth = 1;
    int m_rawHeight = 1;
    int m_rawByteSize = 16;
};
