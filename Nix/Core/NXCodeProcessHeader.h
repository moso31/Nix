#pragma once
#include <vector>
#include <string>
#include "Ntr.h"
#include "BaseDefs/Math.h"

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

struct NXGUIDataCBuffer
{
	NXGUICBufferStyle style;
	Vector2 params; // gui拖动参数附加属性，like drugspeed, sliderMin/Max..
};

struct NXMatDataCBuffer;
struct NXMatDataTexture;
struct NXMatDataSampler;
struct NXMatDataBase
{
	virtual ~NXMatDataBase() = default;

	virtual NXMatDataCBuffer* IsCBuffer() { return nullptr; }
	virtual NXMatDataTexture* IsTexture() { return nullptr; }
	virtual NXMatDataSampler* IsSampler() { return nullptr; }
	virtual void CopyFrom(const NXMatDataBase* pCopy) = 0;
	virtual void SyncLink() = 0; // 数据同步到材质

	std::string name;
	NXMatDataBase* pFastLink = nullptr; 
};

struct NXMatDataCBuffer : public NXMatDataBase
{
	NXMatDataCBuffer* IsCBuffer() override { return this; }
	virtual void SyncLink() override // 数据同步到材质
	{
		if (!pFastLink) return;
		if (auto* linkData = pFastLink->IsCBuffer())
		{
			linkData->data = data;
			linkData->size = size;
		}
	};
	virtual void CopyFrom(const NXMatDataBase* pCopy) override
	{
		auto* pCBuffer = static_cast<const NXMatDataCBuffer*>(pCopy);
		*this = *pCBuffer;
	}

	Vector4 data;
	int size;
	NXGUIDataCBuffer gui;
};

struct NXMatDataTexture : public NXMatDataBase
{
	NXMatDataTexture* IsTexture() override { return this; }
	virtual void CopyFrom(const NXMatDataBase* pCopy) override
	{
		auto* pTexture = static_cast<const NXMatDataTexture*>(pCopy);
		*this = *pTexture;
	}
	virtual void SyncLink() override // 数据同步到材质
	{
		if (!pFastLink) return;
		if (auto* linkData = pFastLink->IsTexture())
		{
			linkData->pTexture = pTexture;
		}
	}

	Ntr<NXTexture> pTexture;
};

struct NXMatDataSampler : public NXMatDataBase
{
	NXMatDataSampler* IsSampler() override { return this; }
	virtual void CopyFrom(const NXMatDataBase* pCopy) override
	{
		auto* pSampler = static_cast<const NXMatDataSampler*>(pCopy);
		*this = *pSampler;
	}
	virtual void SyncLink() override // 数据同步到材质
	{
		if (!pFastLink) return;
		if (auto* linkData = pFastLink->IsSampler())
		{
			linkData->filter = filter;
			linkData->addressU = addressU;
			linkData->addressV = addressV;
			linkData->addressW = addressW;
		}
	}

	NXSamplerFilter filter;
	NXSamplerAddressMode addressU;
	NXSamplerAddressMode addressV;
	NXSamplerAddressMode addressW;
};

struct NXMatDataSettings
{
	uint32_t shadingModel = 0;
};

class NXMaterialData
{
public:
	void AddCBuffer(NXMatDataCBuffer* cb) { cbArr.push_back(cb); allArr.push_back(cb); }
	void AddTexture(NXMatDataTexture* tx) { txArr.push_back(tx); allArr.push_back(tx); }
	void AddSampler(NXMatDataSampler* ss) { ssArr.push_back(ss); allArr.push_back(ss); }

	// clone = 基本深拷贝（除了texture指针等）
	NXMaterialData Clone() const
	{
		NXMaterialData newData;

		for (auto* cb : cbArr)
		{
			auto newCB = new NXMatDataCBuffer();
			*newCB = *cb;
			newData.AddCBuffer(newCB);
		}
		for (auto* tx : txArr)
		{
			auto newTX = new NXMatDataTexture();
			*newTX = *tx;
			newData.AddTexture(newTX);
		}
		for (auto* ss : ssArr)
		{
			auto newSS = new NXMatDataSampler();
			*newSS = *ss;
			newData.AddSampler(newSS);
		}

		newData.settings = settings;
		return newData;
	}

	void Destroy()
	{
		for (auto& cb : cbArr) delete cb;
		for (auto& tx : txArr) delete tx;
		for (auto& ss : ssArr) delete ss;
		cbArr.clear();
		txArr.clear();
		ssArr.clear();
		allArr.clear();
	}

	void Remove(NXMatDataBase* data)
	{
		if (data->IsCBuffer()) std::erase_if(cbArr, [data](NXMatDataCBuffer* d) { return d == data; });
		else if (data->IsTexture()) std::erase_if(txArr, [data](NXMatDataTexture* d) { return d == data; });
		else if (data->IsSampler()) std::erase_if(ssArr, [data](NXMatDataSampler* d) { return d == data; });
		std::erase_if(allArr, [data](NXMatDataBase* d) { return d == data; });
		delete data;
	}

	NXMatDataCBuffer* FindCBufferByName(const std::string& name)
	{
		auto it = std::find_if(cbArr.begin(), cbArr.end(), [name](NXMatDataCBuffer* d) { return d->name == name; });
		if (it != cbArr.end())
			return *it;
		return nullptr;
	}

	NXMatDataTexture* FindTextureByName(const std::string& name)
	{
		auto it = std::find_if(txArr.begin(), txArr.end(), [name](NXMatDataTexture* d) { return d->name == name; });
		if (it != txArr.end())
			return *it;
		return nullptr;
	}

	NXMatDataSampler* FindSamplerByName(const std::string& name)
	{
		auto it = std::find_if(ssArr.begin(), ssArr.end(), [name](NXMatDataSampler* d) { return d->name == name; });
		if (it != ssArr.end())
			return *it;
		return nullptr;
	}

	NXMatDataTexture* FindTexture(const Ntr<NXTexture>& pTex)
	{
		auto it = std::find_if(txArr.begin(), txArr.end(), [pTex](Ntr<NXTexture>& t) { return t == pTex; });
		if (it != txArr.end())
			return *it;
		return nullptr;
	}

	void MoveToPrev(NXMatDataBase* data)
	{
		auto it = std::find_if(allArr.begin(), allArr.end(), [data](NXMatDataBase* d) { return d == data; });

		if (it != allArr.end() && it != allArr.begin())
		{
			auto itPrev = it - 1;
			std::iter_swap(it, itPrev);

			if ((*it)->IsCBuffer() && (*itPrev)->IsCBuffer())
			{
				auto it0 = std::find_if(cbArr.begin(), cbArr.end(), [data](NXMatDataCBuffer* d) { return d == data; });
				std::iter_swap(it0, it0 - 1);
			}
			else if ((*it)->IsTexture() && (*itPrev)->IsTexture())
			{
				auto it0 = std::find_if(txArr.begin(), txArr.end(), [data](NXMatDataTexture* d) { return d == data; });
				std::iter_swap(it0, it0 - 1);
			}
			else if ((*it)->IsSampler() && (*itPrev)->IsSampler())
			{
				auto it0 = std::find_if(ssArr.begin(), ssArr.end(), [data](NXMatDataSampler* d) { return d == data; });
				std::iter_swap(it0, it0 - 1);
			}
		}
	}

	void MoveToNext(NXMatDataBase* data)
	{
		auto it = std::find_if(allArr.begin(), allArr.end(), [data](NXMatDataBase* d) { return d == data; });

		if (it != allArr.end() && it + 1 != allArr.end())
		{
			auto itNext = it + 1;
			std::iter_swap(it, itNext);

			if ((*it)->IsCBuffer() && (*itNext)->IsCBuffer())
			{
				auto it0 = std::find_if(cbArr.begin(), cbArr.end(), [data](NXMatDataCBuffer* d) { return d == data; });
				std::iter_swap(it0, it0 + 1);
			}
			else if ((*it)->IsTexture() && (*itNext)->IsTexture())
			{
				auto it0 = std::find_if(txArr.begin(), txArr.end(), [data](NXMatDataTexture* d) { return d == data; });
				std::iter_swap(it0, it0 + 1);
			}
			else if ((*it)->IsSampler() && (*itNext)->IsSampler())
			{
				auto it0 = std::find_if(ssArr.begin(), ssArr.end(), [data](NXMatDataSampler* d) { return d == data; });
				std::iter_swap(it0, it0 + 1);
			}
		}
	}

	void MoveToBegin(NXMatDataBase* data)
	{
		auto it = std::find_if(allArr.begin(), allArr.end(), [data](NXMatDataBase* d) { return d == data; });
		if (it != allArr.end() && it != allArr.begin())
		{
			std::rotate(allArr.begin(), it, it + 1);

			if ((*it)->IsCBuffer())
			{
				auto it0 = std::find_if(cbArr.begin(), cbArr.end(), [data](NXMatDataCBuffer* d) { return d == data; });
				std::rotate(cbArr.begin(), it0, it0 + 1);
			}
			else if ((*it)->IsTexture())
			{
				auto it0 = std::find_if(txArr.begin(), txArr.end(), [data](NXMatDataTexture* d) { return d == data; });
				std::rotate(txArr.begin(), it0, it0 + 1);
			}
			else if ((*it)->IsSampler())
			{
				auto it0 = std::find_if(ssArr.begin(), ssArr.end(), [data](NXMatDataSampler* d) { return d == data; });
				std::rotate(ssArr.begin(), it0, it0 + 1);
			}
		}
	}

	void MoveToEnd(NXMatDataBase* data)
	{
		auto it = std::find_if(allArr.begin(), allArr.end(), [data](NXMatDataBase* d) { return d == data; });
		if (it != allArr.end() && it + 1 != allArr.end())
		{
			std::rotate(it, it + 1, allArr.end());

			if ((*it)->IsCBuffer())
			{
				auto it0 = std::find_if(cbArr.begin(), cbArr.end(), [data](NXMatDataCBuffer* d) { return d == data; });
				std::rotate(it0, it0 + 1, cbArr.end());
			}
			else if ((*it)->IsTexture())
			{
				auto it0 = std::find_if(txArr.begin(), txArr.end(), [data](NXMatDataTexture* d) { return d == data; });
				std::rotate(it0, it0 + 1, txArr.end());
			}
			else if ((*it)->IsSampler())
			{
				auto it0 = std::find_if(ssArr.begin(), ssArr.end(), [data](NXMatDataSampler* d) { return d == data; });
				std::rotate(it0, it0 + 1, ssArr.end());
			}
		}
	}

	const std::vector<NXMatDataCBuffer*>& GetCBuffers() const { return cbArr; }
	const std::vector<NXMatDataTexture*>& GetTextures() const { return txArr; }
	const std::vector<NXMatDataSampler*>& GetSamplers() const { return ssArr; }
	const std::vector<NXMatDataBase*>& GetAll() { return allArr; }
	NXMatDataSettings& Settings() { return settings; }

private:
	std::vector<NXMatDataCBuffer*> cbArr;
	std::vector<NXMatDataTexture*> txArr;
	std::vector<NXMatDataSampler*> ssArr;
	std::vector<NXMatDataBase*> allArr;
	NXMatDataSettings settings;
};

struct NXMaterialPassCode
{
	std::string name;
	std::string vsFunc;
	std::string psFunc;
};

struct NXMaterialFunctionsCode
{
	std::vector<std::string> data; // 整个函数
	std::vector<std::string> title; // 仅第一行
};

struct NXMaterialCode
{
	std::string shaderName;
	NXMaterialFunctionsCode commonFuncs; // 通用函数代码
	std::vector<NXMaterialPassCode> passes; // pass代码【todo: multipasses扩展，现在只有单pass】
};
