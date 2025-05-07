#pragma once
#include <vector>
#include <string>
#include "Ntr.h"

class NXTexture;

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

// material copyData

enum class NXMaterialBaseType
{
	CBuffer,
	Texture,
	Sampler
};

struct NXMaterialBaseData 
{
	NXMaterialBaseData(const std::string& name, NXMaterialBaseType type) : name(name), baseType(type) {}
	virtual ~NXMaterialBaseData() {}

	std::string name;

	NXMaterialBaseType GetType() const { return baseType; }

protected:
	NXMaterialBaseType baseType;
};

struct NXMaterialData_CBuffer : public NXMaterialBaseData
{
	NXMaterialData_CBuffer(const std::string& name) : NXMaterialBaseData(name, NXMaterialBaseType::CBuffer), data(0.0f), size(0) {}
	NXMaterialData_CBuffer(const std::string& name, const Vector4& data, int size) : NXMaterialBaseData(name, NXMaterialBaseType::CBuffer), data(data), size(size) {}

	// ��¼ gui�� CB����ÿ�����ݶ�ʹ������ Vec4 ���档
	// ��ô����Ϊ�˱��� GUI �����ı����ݸ�ʽʱ������������ڴ���䡣
	Vector4 data;
	int size;
};

struct NXMaterialData_Texture : public NXMaterialBaseData
{
	NXMaterialData_Texture(const std::string& name) : NXMaterialBaseData(name, NXMaterialBaseType::Texture), pTexture(nullptr) {}
	NXMaterialData_Texture(const std::string& name, const Ntr<NXTexture>& pTexture) : NXMaterialBaseData(name, NXMaterialBaseType::Texture), pTexture(pTexture) {}
	Ntr<NXTexture> pTexture;
};

struct NXMaterialData_Sampler : public NXMaterialBaseData
{
	NXMaterialData_Sampler(const std::string& name) : NXMaterialBaseData(name, NXMaterialBaseType::Sampler), filter(NXSamplerFilter::Unknown), addressU(NXSamplerAddressMode::Unknown), addressV(NXSamplerAddressMode::Unknown), addressW(NXSamplerAddressMode::Unknown) {}
	NXMaterialData_Sampler(const std::string& name, const NXSamplerFilter& filter, const NXSamplerAddressMode& addressU, const NXSamplerAddressMode& addressV, const NXSamplerAddressMode& addressW) : NXMaterialBaseData(name, NXMaterialBaseType::Sampler), filter(filter), addressU(addressU), addressV(addressV), addressW(addressW) {}
	NXSamplerFilter filter;
	NXSamplerAddressMode addressU;
	NXSamplerAddressMode addressV;
	NXSamplerAddressMode addressW;
};

struct NXMaterialData_CBufferSets
{
	uint32_t shadingModel = 0;
};

struct NXMaterialData
{
	void Destroy()
	{
		for (auto& data : datas)
		{
			delete data;
		}
		datas.clear();
	}

	std::vector<NXMaterialBaseData*> datas;
	NXMaterialData_CBufferSets sets;
};

// NXMSE = NXMaterial Shader Editor(GUI datas)

enum class NXMSE_CBufferStyle
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
	NXMSE_CBufferStyle style;

	// gui�϶������������ԣ�like drugspeed, sliderMin/Max..
	float guiParams0;
	float guiParams1;
};

struct NXMSE_BaseData
{
	virtual NXMSE_BaseData* Clone() = 0;
	virtual void Destroy()
	{
		delete pMaterialData;
	}

	// MSE/GUI�Լ������ݣ����ڱ���ǰ��AddParam��backup
	NXMaterialBaseData* pMaterialData = nullptr;

	// ���ʱ���ʹ�õ����ݣ�MSE/GUIֵ�޸�ʱ��ʱ����Ч����
	NXMaterialBaseData* pMaterialLink = nullptr;
};

struct NXMSE_CBufferData : public NXMSE_BaseData
{
	NXMaterialData_CBuffer* MaterialData() { return (NXMaterialData_CBuffer*)(pMaterialData); }
	NXMaterialData_CBuffer* MaterialLink() { return (NXMaterialData_CBuffer*)(pMaterialLink); }

	virtual NXMSE_BaseData* Clone() override
	{
		NXMSE_CBufferData* pData = new NXMSE_CBufferData();
		pData->pMaterialData = new NXMaterialData_CBuffer(MaterialData()->name, MaterialData()->data, MaterialData()->size);
		pData->pMaterialLink = pMaterialLink;
		pData->guiStyle = guiStyle;
		pData->guiParams = guiParams;
		return pData;
	}

	NXMSE_CBufferStyle guiStyle;
	Vector2 guiParams;
};

struct NXMSE_TextureData : public NXMSE_BaseData
{
	NXMaterialData_Texture* MaterialData() { return (NXMaterialData_Texture*)(pMaterialData); }
	NXMaterialData_Texture* MaterialLink() { return (NXMaterialData_Texture*)(pMaterialLink); }

	virtual NXMSE_BaseData* Clone() override
	{
		NXMSE_TextureData* pData = new NXMSE_TextureData();
		pData->pMaterialData = new NXMaterialData_Texture(MaterialData()->name, MaterialData()->pTexture);
		pData->pMaterialLink = pMaterialLink;
		return pData;
	}
};

struct NXMSE_SamplerData : public NXMSE_BaseData
{
	NXMaterialData_Sampler* MaterialData() { return (NXMaterialData_Sampler*)(pMaterialData); }
	NXMaterialData_Sampler* MaterialLink() { return (NXMaterialData_Sampler*)(pMaterialLink); }

	virtual NXMSE_BaseData* Clone() override
	{
		NXMSE_SamplerData* pData = new NXMSE_SamplerData();
		pData->pMaterialData = new NXMaterialData_Sampler(MaterialData()->name, MaterialData()->filter, MaterialData()->addressU, MaterialData()->addressV, MaterialData()->addressW);
		pData->pMaterialLink = pMaterialLink;
		return pData;
	}
};

struct NXMSE_SettingsData 
{
	uint32_t shadingModel = 0;
};

struct NXMSEPackDatas
{
	NXMSEPackDatas Clone() const
	{
		NXMSEPackDatas clone;
		for (auto& data : datas)
		{
			clone.datas.push_back(data->Clone());
		}
		clone.settings = settings;
		return clone;
	}

	void Destroy()
	{
		for (auto& data : datas)
		{
			data->Destroy();
		}
		datas.clear();
	}

	std::vector<NXMSE_BaseData*> datas; // ��������
	NXMSE_SettingsData settings; // ����ʵ������
};

struct NXMaterialPassCode
{
	std::string name;
	std::string vsFunc;
	std::string psFunc;
};

struct NXMaterialFunctionsCode
{
	std::vector<std::string> data; // ��������
	std::vector<std::string> title; // ����һ��
};

struct NXMaterialCode
{
	std::string shaderName;
	NXMaterialFunctionsCode commonFuncs; // ͨ�ú�������
	std::vector<NXMaterialPassCode> passes; // pass���롾todo: multipasses��չ������ֻ�е�pass��
};
