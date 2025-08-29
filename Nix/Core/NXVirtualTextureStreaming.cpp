#include "NXVirtualTextureStreaming.h"
#include "NXGlobalDefinitions.h"

void NXVirtualTextureStreaming::Init()
{
	auto pDevice = NXGlobalDX::GetDevice();
	NX12Util::CreateCommands(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_pCmdAllocator.GetAddressOf(), m_pCmdList.GetAddressOf());
	m_pCmdList->SetName(L"VirtualTexture Streaming Command List");
	m_pCmdList->Reset(m_pCmdAllocator.Get(), nullptr);
}

void NXVirtualTextureStreaming::Update()
{
	std::string str = "Bake Texture";
	NX12Util::BeginEvent(m_pCmdList.Get(), str.c_str());



	NX12Util::EndEvent(m_pCmdList.Get());
}
