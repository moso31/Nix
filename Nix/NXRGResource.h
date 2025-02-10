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
	// �Ƿ�ʹ��view�ķֱ���
	bool isDynamicResolution = true;

	union
	{
		// isDynamicResolution == true ʱ
		// �ֱ��ʱ���������pass�ķֱ������ŵ�0.5x, 0.25x��
		float dynamicResolutionRatio = 1.0f;

		// isDynamicResolution == false ʱ
		struct
		{
			// �����size
			uint32_t width;
			uint32_t height;
		};
	};

	// �����DXGI��ʽ
	DXGI_FORMAT format;

	// ������RT����DS�������RT���ж��ţ������DSֻ�ܷ�һ��
	NXRGHandleFlags handleFlags;
};

class NXTexture;
class NXRGResource
{
public:
	NXRGResource(NXRGResource* pOldResource);
	NXRGResource(const std::string& name, const NXRGDescription& description);

	const std::string& GetName() { return m_name; }

	NXRGHandle* GetHandle() { return m_handle; }
	const NXRGDescription& GetDescription() { return m_description; }

	bool HasWrited() { return m_bHasWrited; }
	void MakeWriteConnect() { m_bHasWrited = true; }

	void SetResource(Ntr<NXTexture> pResource) { m_pResource = pResource; }
	Ntr<NXTexture> GetResource() { return m_pResource; }

private:
	std::string m_name;
	bool m_bHasWrited;

	NXRGHandle* m_handle;
	NXRGDescription m_description;

	Ntr<NXTexture> m_pResource;
};

