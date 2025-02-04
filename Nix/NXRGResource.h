#pragma once
#include "NXRGHandle.h"

enum NXRGHandleType
{
	RG_Texture2D,
	RG_TextureArray
};

enum NXRGHandleFlags
{
	RG_RenderTarget,
	RG_DepthStencil
};

struct NXRGDescription
{
	uint32_t width;
	uint32_t height;
	DXGI_FORMAT format;
	NXRGHandleFlags handleFlags;
	NXRGHandleType handleType;
};

class NXTexture;
class NXRGResource
{
public:
	NXRGResource(NXRGHandle* handle);
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

