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
		// 如果之前没被写入过，可以直接用
		m_outputs.push_back(pResource);
		pResource->MakeWriteConnect(); // 标记为已写入
		return pResource;
	}
	else
	{
		// 创建新版本
		NXRGResource* pNewVersionResource = new NXRGResource(pResource->GetHandle());
		m_outputs.push_back(pNewVersionResource);
		return pNewVersionResource;
	}
}

void NXRGPassNode::Execute()
{
}
