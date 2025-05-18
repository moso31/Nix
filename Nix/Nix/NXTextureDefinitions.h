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
    Raw,			// ʹ��ԭ����ʽ
    sRGB,           // sRGB��ɫ����
    Linear,         // ������ɫ����
    NormalMap,      // ������ͼ,
    Count
};

struct NXTextureSerializationData
{
    // ��������
    NXTextureMode m_textureType = NXTextureMode::Raw;

    // �Ƿ�ת����Y��
    bool m_bInvertNormalY = false;

    // �Ƿ�����mipmap
    bool m_bGenerateMipMap = true;

    // �Ƿ�����������ͼ
    bool m_bCubeMap = false;

    // ��.raw�ļ�ʹ�ã���¼raw��ʽ�Ŀ�ߺ��ֽڴ�С
    int m_rawWidth = 1;
    int m_rawHeight = 1;
    int m_rawByteSize = 16;
};
