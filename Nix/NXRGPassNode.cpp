#include "NXRGPassNode.h"
#include "NXRGHandle.h"
#include "NXRendererPass.h"

NXRGResource* NXRGPassNode::Create(const NXRGDescription& desc)
{
	m_pRenderGraph->AddResource(new NXRGResource(desc));
}

void NXRGPassNode::Read(NXRGResource* pResource)
{
	m_inputs.push_back(pResource);
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
		pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
		m_pRenderGraph->AddResource(pNewVersionResource); // ��ӵ�graph��
		return pNewVersionResource;
	}
}

void NXRGPassNode::Compile()
{
	for (int i = 0; i < m_inputs.size(); i++)
	{
		m_pPass->SetInputTex(i, m_inputs[i]->GetResource());
	}

	for (int i = 0; i < m_outputs.size(); i++)
	{
		auto flag = m_outputs[i]->GetDescription().handleFlags;
		if (flag == RG_RenderTarget)
		{
			m_pPass->SetOutputRT(i, m_outputs[i]->GetResource());
		}
		else if (flag == RG_DepthStencil)
		{
			m_pPass->SetOutputDS(m_outputs[i]->GetResource());
		}
	}

	m_setupFunc();
}

void NXRGPassNode::Execute()
{
}
