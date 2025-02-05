#include "NXRGPassNode.h"
#include "NXRGHandle.h"
#include "NXRendererPass.h"

NXRGResource* NXRGPassNode::Create(const NXRGDescription& desc)
{
	NXRGResource* pResource = new NXRGResource(desc);
	m_pRenderGraph->AddResource(pResource);
	return pResource;
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
		NXRGResource* pNewVersionResource = new NXRGResource(pResource);
		m_outputs.push_back(pNewVersionResource);
		pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
		m_pRenderGraph->AddResource(pNewVersionResource); // ��ӵ�graph��
		return pNewVersionResource;
	}
}

void NXRGPassNode::ClearRT(ID3D12GraphicsCommandList* pCmdList, NXRGResource* pResource)
{
	auto& pResDesc = pResource->GetDescription();
	if (pResDesc.handleFlags == RG_RenderTarget)
	{
		pCmdList->ClearRenderTargetView(pResource->GetResource()->GetRTV(), Colors::Black, 0, nullptr);
		return;
	}

	if (pResDesc.handleFlags == RG_DepthStencil)
	{
		pCmdList->ClearDepthStencilView(pResource->GetResource()->GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0x0, 0, nullptr);
		return;
	}
}

void NXRGPassNode::SetViewPortAndScissorRect(ID3D12GraphicsCommandList* pCmdList, const Vector2& size)
{
	auto vpCamera = NX12Util::ViewPort(size.x, size.y);
	pCmdList->RSSetViewports(1, &vpCamera);
	pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));
}

void NXRGPassNode::Compile()
{
	for (auto pInRes : m_inputs)
	{
		m_pPass->PushInputTex(pInRes->GetResource());
	}

	for (auto pOutRes : m_outputs)
	{
		auto flag = pOutRes->GetDescription().handleFlags;
		if (flag == RG_RenderTarget)
		{
			m_pPass->PushOutputRT(pOutRes->GetResource());
		}
		else if (flag == RG_DepthStencil)
		{
			m_pPass->SetOutputDS(pOutRes->GetResource());
		}
	}

	m_setupFunc();
	m_pPass->SetupInternal();
}

void NXRGPassNode::Execute(ID3D12GraphicsCommandList* pCmdList)
{
	m_executeFunc(pCmdList);
	m_pPass->Render(pCmdList);
}
