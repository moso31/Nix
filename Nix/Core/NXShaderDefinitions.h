#pragma once
#include "Header.h"

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
    NXTexture* pTexture;
    UINT slotIndex;
};

struct NXMaterialSamplerInfo
{
    std::string name;
    ComPtr<ID3D11SamplerState> pSampler;
    UINT slotIndex;
};

// �� GUI �е���ʾ Style
enum class NXGUICBufferStyle
{
	Value,
	Value2,
	Value3,
	Value4,
	Slider,
	Slider2,
	Slider3,
	Slider4,
	Color3,
	Color4,
	Unknown
};

struct NXGUICBufferData
{
	std::string name;

	// ��ȡ CB ʱ�ĳ�ʼTypeֵ
	// �� Param �� Type �仯��ʱ�򣬿��Ա��� copy ���࣬����ָ��ƫ�ơ�
	NXCBufferInputType readType;

	// ��¼ CBֵ����ÿ�����ݶ�ʹ������ Vec4 ���档
	// ��ô����Ϊ�˱��� GUI �ı����ݸ�ʽ����������ڴ���䡣
	Vector4 data;

	// CB �� GUI �������ʾ
	NXGUICBufferStyle guiStyle;

	// CB �� GUI �еĸ���������������������GUI��drugSpeed, sliderMin/Max�ȵȡ�
	Vector2 params;

	// ��¼ ԭCB �� memoryIndex
	// �� GUI ֵ�����ʱ��ʹ�ô���������׷�ݲ����е�Դ����ָ�롣
	int memoryIndex;
};

struct NXGUITextureData
{
	std::string name;
	NXTexture* pTexture;
};

struct NXGUISamplerData
{
	std::string name;
	ComPtr<ID3D11SamplerState> pSampler;
};

extern const char* g_strCBufferGUIStyle[];
extern const int g_strCBufferGUIStyleCount;