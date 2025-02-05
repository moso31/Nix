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
	// �����size
	uint32_t width;
	uint32_t height;

	// �����DXGI��ʽ
	DXGI_FORMAT format;

	// ������RT����DS�������RT���ж��ţ������DSֻ�ܷ�һ��
	NXRGHandleFlags handleFlags;

	// �������͡�TODO ��������𣿡�
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

