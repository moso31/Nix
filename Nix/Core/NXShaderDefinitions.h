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

// 在 GUI 中的显示 Style
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

	// 读取 CB 时的初始Type值
	// 当 Param 的 Type 变化的时候，可以避免 copy 过多，导致指针偏移。
	NXCBufferInputType readType;

	// 记录 CB值，但每个数据都使用最大的 Vec4 储存。
	// 这么做是为了避免 GUI 改变数据格式产生额外的内存分配。
	Vector4 data;

	// CB 在 GUI 中如何显示
	NXGUICBufferStyle guiStyle;

	// CB 在 GUI 中的辅助参数，比如用来控制GUI的drugSpeed, sliderMin/Max等等。
	Vector2 params;

	// 记录 原CB 的 memoryIndex
	// 当 GUI 值变更的时候，使用此索引就能追溯材质中的源数据指针。
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