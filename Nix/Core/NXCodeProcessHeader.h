#pragma once
#include <vector>
#include <string>
#include "Ntr.h"

class NXTexture;

enum class NXCBufferInputType
{
	None = 0,
	Float = 1,
	Float2 = 2,
	Float3 = 3,
	Float4 = 4,
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

// shadercode

struct NXShaderBlockTexture
{
	std::string type;
	std::string name;
};

struct NXShaderBlockSampler
{
	std::string type;
	std::string name;
};

struct NXShaderBlockValue
{
	std::string type;
	std::string name;
};

struct NXShaderBlockConstantBuffer
{
	std::vector<NXShaderBlockValue> values;
};

struct NXShaderBlockParams
{
	std::vector<NXShaderBlockTexture> textures;
	std::vector<NXShaderBlockSampler> samplers;
	NXShaderBlockConstantBuffer cbuffer;
};

struct NXShaderBlockPass
{
	std::string name;
	std::string vsFunc;
	std::string psFunc;
};

struct NXShaderBlockSubShader
{
	std::vector<NXShaderBlockPass> passes;
};

struct NXShaderBlockFuncBody
{
	std::string title;
	std::string data;
};

struct NXShaderBlockFunctions
{
	std::vector<NXShaderBlockFuncBody> bodys;
};

struct NXShaderBlock
{
	std::string name;
	NXShaderBlockParams params;
	NXShaderBlockFunctions globalFuncs;
	NXShaderBlockSubShader subShader;
};

// block

struct NXMaterialData_CBufferItem
{
	std::string name;
	NXCBufferInputType type;

	// ��¼ gui�� CB����ÿ�����ݶ�ʹ������ Vec4 ���档
	// ��ô����Ϊ�˱��� GUI �����ı����ݸ�ʽʱ������������ڴ���䡣
	float value[4];

	NXGUIStyle_CBufferItem guiStyle;
};

struct NXMaterialData_CBufferSets
{
	uint32_t shadingModel = 0;
};

struct NXMaterialData_Sampler
{
	std::string name;
	NXSamplerFilter filter;
	NXSamplerAddressMode addressU;
	NXSamplerAddressMode addressV;
	NXSamplerAddressMode addressW;
};

struct NXMaterialData_Texture
{
	std::string name;
	Ntr<NXTexture> pTexture;
};

struct NXMaterialData_CBuffer
{
	std::vector<NXMaterialData_CBufferItem> elems; // ����cb����

	// ��������
	// 2023.8.13 Ŀǰ����ֻ��¼��ʹ�õĹ���ģ�͡����������и�����չ�����米���޳�ɶ�ġ�
	NXMaterialData_CBufferSets sets;
};

struct NXMaterialData
{
	void Clear()
	{
		cbuffer.elems.clear();
	}

	NXMaterialData_Sampler sampler;
	NXMaterialData_Texture texture;
	NXMaterialData_CBuffer cbuffer;
};

enum class NXGUIStyle_CBufferType
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

struct NXGUIStyle_CBufferItem
{
	NXGUIStyle_CBufferType style;

	// gui�϶������������ԣ�like drugspeed, sliderMin/Max..
	float guiParams0;
	float guiParams1;
};