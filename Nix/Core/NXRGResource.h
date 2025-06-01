#pragma once
#include "NXRGHandle.h"
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
	bool isViewRT = true; // �Ƿ�ʹ��view�ķֱ���
	float RTScale = 1.0f; // isViewRT == true ʱ�ֱ��ʱ���������pass�ķֱ������ŵ�0.5x, 0.25x��

	bool isImported = false;
	struct
	{
		Ntr<NXResource> pImportResource = nullptr; // ����ǵ�����Դ(isImported)
		uint32_t width;
		uint32_t height;
		uint32_t arraySize;
	} importData;

	NXResourceType type = NXResourceType::Tex2D;

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

	void SetResource(Ntr<NXResource> pResource) { m_pResource = pResource; }
	Ntr<NXResource> GetResource() const { return m_pResource; }

private:
	std::string m_name;
	bool m_bHasWrited;

	NXRGHandle* m_handle;
	NXRGDescription m_description;

	Ntr<NXResource> m_pResource;
};

