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

// 专门用于在材质编辑器下显示纹理类型的枚举，只有 Default 和 Normal 两种选项
enum class NXGUITextureType
{
	Default,
	Normal,
	Unknown
};

enum class NXSamplerFilter
{
	Point,
	Linear,
	Anisotropic,
	Unknown
};

enum class NXSamplerAddressMode
{
	Wrap,
	Mirror,
	Clamp,
	Border,
	MirrorOnce,
	Unknown
};

struct NXCBufferElem
{
    std::string name;
    NXCBufferInputType type;
    int memoryIndex;
	NXGUICBufferStyle style;
	Vector2 guiParams; // gui拖动参数附加属性，drugspeed, sliderMin/Max
};

struct NXCBufferSets
{
	UINT shadingModel;
};

struct NXMaterialCBufferInfo
{
    std::vector<NXCBufferElem> elems;
	NXCBufferSets sets;
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
    UINT slotIndex;
	NXSamplerFilter filter;
	NXSamplerAddressMode addressU;
	NXSamplerAddressMode addressV;
	NXSamplerAddressMode addressW;
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

	// 记录 backup 的 index
	// GUI Revert 时使用此值回复
	int backupIndex = -1;
};

struct NXGUICBufferSetsData
{
	NXCBufferSets data;
};

struct NXGUITextureData
{
	std::string name;
	NXGUITextureType texType;
	NXTexture* pTexture;

	int backupIndex = -1;
};

struct NXGUISamplerData
{
	std::string name;
	NXSamplerFilter filter;
	NXSamplerAddressMode addressU;
	NXSamplerAddressMode addressV;
	NXSamplerAddressMode addressW;

	int backupIndex = -1;
};

// 记录一段 nsl 函数 映射到 HLSL 以后的代码坐标
// （从多少行到多少行）
struct NXHLSLCodeRegion
{
	int firstRow = 0;
	int lastRow = 0;
};

extern const char* g_strCBufferGUIStyle[];
extern const int g_strCBufferGUIStyleCount;