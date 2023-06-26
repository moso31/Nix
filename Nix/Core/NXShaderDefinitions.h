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

// ר�������ڲ��ʱ༭������ʾ�������͵�ö�٣�ֻ�� Default �� Normal ����ѡ��
enum class NXGUITextureType
{
	Default,
	Normal,
	Unknown
};

struct NXCBufferElem
{
    std::string name;
    NXCBufferInputType type;
    int memoryIndex;
	NXGUICBufferStyle style;
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
	NXGUITextureType guiType;
};

struct NXMaterialSamplerInfo
{
    std::string name;
    ComPtr<ID3D11SamplerState> pSampler;
    UINT slotIndex;
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
	NXGUITextureType texType;
	NXTexture* pTexture;
};

struct NXGUISamplerData
{
	std::string name;
	ComPtr<ID3D11SamplerState> pSampler;
};

extern const char* g_strCBufferGUIStyle[];
extern const int g_strCBufferGUIStyleCount;