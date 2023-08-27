#pragma once

enum class NXTextureType
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
    NXTextureType m_textureType = NXTextureType::Raw;

    // �Ƿ�ת����Y��
    bool m_bInvertNormalY = false;

    // �Ƿ�����mipmap
    bool m_bGenerateMipMap = true;

    // �Ƿ�����������ͼ
    bool m_bCubeMap = false;
};
