#pragma once
#include "NXRGHandle.h"

enum NXRGHandleType
{
	RG_Texture2D,
	RG_Texture2DArray
};

enum NXRGHandleFlags
{
	RG_RenderTarget = 1 << 0,
	RG_DepthStencil = 1 << 1
};

struct NXRGDescription
{
	// 纹理的size
	uint32_t width;
	uint32_t height;

	// 纹理的DXGI格式
	DXGI_FORMAT format;

	// 纹理是RT还是DS，如果是RT运行多张；如果是DS只能放一张
	NXRGHandleFlags handleFlags;

	// 纹理类型【TODO 真的有用吗？】
	NXRGHandleType handleType;
};

class NXTexture;
class NXRGResource
{
public:
	NXRGResource(NXRGResource* pOldResource);
	NXRGResource(const NXRGDescription& description);

	NXRGHandle* GetHandle() { return m_handle; }
	const NXRGDescription& GetDescription() { return m_description; }

	bool HasWrited() { return m_bHasWrited; }
	bool MakeWriteConnect() { m_bHasWrited = true; }

	void SetResource(Ntr<NXTexture> pResource) { m_pResource = pResource; }
	Ntr<NXTexture> GetResource() { return m_pResource; }

private:
	bool m_bHasWrited;

	NXRGHandle* m_handle;
	NXRGDescription m_description;

	Ntr<NXTexture> m_pResource;
};

