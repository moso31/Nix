#pragma once
#include "NXRGResourceVersion.h"
#include "NXTexture.h"
#include "NXBuffer.h"

enum NXRGHandleType
{
	RG_Texture2D,
	RG_Texture2DArray
};

enum NXRGHandleFlags
{
	RG_None = 0,
	RG_RenderTarget = 1 << 0,
	RG_DepthStencil = 1 << 1
};

struct NXRGDescription
{
	bool isViewRT = true; // 是否使用view的分辨率
	float RTScale = 1.0f; // isViewRT == true 时分辨率比例。允许pass的分辨率缩放到0.5x, 0.25x等

	// 只有缓冲使用这个参数，单个数据的字节量，默认使用sizeof(int)=4
	int stride = 4; 

	bool isImported = false;
	struct
	{
		Ntr<NXResource> pImportResource = nullptr; // 如果是导入资源(isImported)
		uint32_t width;
		uint32_t height;
		uint32_t arraySize;
	} importData;

	// 资源类型
	NXResourceType type = NXResourceType::Tex2D;

	// 资源DXGI格式
	DXGI_FORMAT format;

	//（纹理）是RT还是DS
	NXRGHandleFlags handleFlags;
};

class NXTexture;
class NXRGResource
{
public:
	NXRGResource(NXRGResource* pOldResource);
	NXRGResource(const std::string& name, const NXRGDescription& description);

	const std::string& GetName() { return m_name; }

	NXRGResourceVersion* GetVersion() { return m_version; }
	const NXRGDescription& GetDescription() { return m_description; }

	bool HasWrited() { return m_bHasWrited; }
	void MakeWriteConnect() { m_bHasWrited = true; }

	void SetResource(Ntr<NXResource> pResource) { m_pResource = pResource; }
	Ntr<NXResource> GetResource() const { return m_pResource; }
	Ntr<NXBuffer> GetBuffer() const { return m_pResource.As<NXBuffer>(); }
	Ntr<NXTexture> GetTexture() const { return m_pResource.As<NXTexture>(); }

private:
	std::string m_name;
	bool m_bHasWrited;

	NXRGResourceVersion* m_version;
	NXRGDescription m_description;

	Ntr<NXResource> m_pResource;
};

