#include "NXRGPassNode.h"
#include "NXRGHandle.h"

NXRGResource* NXRGPassNode::Create(const NXRGDescription& desc)
{
	m_pRenderGraph->AddResource(new NXRGResource(desc));
}

NXRGResource* NXRGPassNode::Read(NXRGResource* pResource)
{
	m_inputs.push_back(pResource);
	return pResource;
}

NXRGResource* NXRGPassNode::Write(NXRGResource* pResource)
{
	if (!pResource->HasWrited())
	{
		// ���֮ǰû��д���������ֱ����
		m_outputs.push_back(pResource);
		pResource->MakeWriteConnect(); // ���Ϊ��д��
		return pResource;
	}
	else
	{
		// �����°汾
		NXRGResource* pNewVersionResource = new NXRGResource(pResource->GetHandle());
		m_outputs.push_back(pNewVersionResource);
		return pNewVersionResource;
	}
}

void NXRGPassNode::Execute()
{
}
