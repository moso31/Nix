#include "ConstantBufferAllocator.h"

using namespace ccmem;

CommittedBufferAllocator::CommittedBufferAllocator(ID3D12Device* pDevice, uint32_t blockByteSize, uint32_t fullByteSize) :
	m_fullByteSize(fullByteSize),
	m_pDevice(pDevice),
	BuddyAllocator(blockByteSize, fullByteSize)
{
}

void CommittedBufferAllocator::Alloc(uint32_t byteSize, const std::function<void(const BufferAllocTaskResult&)>& callback)
{
	BuddyAllocator::Alloc(byteSize, [this, callback](const BuddyTaskResult& taskResult) {
		BufferAllocTaskResult result;
		result.cpuAddress = taskResult.pMemory;
		result.gpuAddress = m_pResource->GetGPUVirtualAddress() + (taskResult.pMemory - m_pResourceData);
		callback(result);
	});
}

void ccmem::CommittedBufferAllocator::Free(uint8_t* pMem)
{
	BuddyAllocator::Free(pMem);
}

void ccmem::CommittedBufferAllocator::OnAllocatorAdded()
{
	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC cbDesc;
	cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	cbDesc.Width = m_fullByteSize; // �ܴ�С����λΪ�ֽ�
	cbDesc.Height = 1; // ���ڻ��������߶ȱ���Ϊ1
	cbDesc.DepthOrArraySize = 1; // ���ڻ���������ȱ���Ϊ1
	cbDesc.MipLevels = 1; // ��������MIP�ȼ�����ӦΪ1
	cbDesc.Format = DXGI_FORMAT_UNKNOWN; // ��������ʹ��DXGI��ʽ����������ΪUNKNOWN
	cbDesc.SampleDesc.Count = 1; // ���ز��������������ڻ���������Ӧ����1
	cbDesc.SampleDesc.Quality = 0; // ���ز�����������ͨ��Ϊ0
	cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // �������Ĳ��֣�ͨ��Ϊ������
	cbDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // ��Դ�ı�־��������Ҫ��������

	HRESULT hr = m_pDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc, // ��Դ����
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr,
		IID_PPV_ARGS(&m_pResource)
	);

	// ����ӳ���ַ
	m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pResourceData));

	// ���õ�������Դ����
	m_pResource->SetName(L"CBuffer");
}
