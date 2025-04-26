#pragma once
#include "BaseDefs/Math.h"
#include "Ntr.h"
#include <string>

struct NXCBufferElem
{
    std::string name;
    NXCBufferInputType type;
    int memoryIndex;
	NXGUICBufferStyle style;
	Vector2 guiParams; // gui�϶������������ԣ�drugspeed, sliderMin/Max
};

struct NXMaterialCBufferInfo
{
	// elems: ���ʵĸ��ֲ�����
    std::vector<NXCBufferElem> elems;

	// sets: ���ʵĸ�������
	// 2023.8.13 Ŀǰ����ֻ��¼��ʹ�õĹ���ģ�͡����������и�����չ�����米���޳�ɶ�ġ�
	NXCBufferSets sets;

	// ��������Ԫ�ؽ������ͬһ�� cbuffer �С�
    UINT slotIndex;
};

class NXTexture;
struct NXMaterialTextureInfo
{
    std::string name;
    Ntr<NXTexture> pTexture;
    UINT slotIndex;
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

	// ��¼ backup �� index
	// GUI Revert ʱʹ�ô�ֵ�ظ�
	int backupIndex = -1;
};

struct NXGUICBufferSetsData
{
	NXCBufferSets data;
};

struct NXGUITextureData
{
	std::string name;
	Ntr<NXTexture> pTexture;

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

// ��¼һ�� nsl ���� ӳ�䵽 HLSL �Ժ�Ĵ�������
// ���Ӷ����е������У�
struct NXHLSLCodeRegion
{
	int firstRow = 0;
	int lastRow = 0;
};

extern const char* g_strCBufferGUIStyle[];
extern const int g_strCBufferGUIStyleCount;