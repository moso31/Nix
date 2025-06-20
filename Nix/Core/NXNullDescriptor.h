#pragma once
#include "DescriptorAllocator.h"

using namespace ccmem;

class NXNullDescriptor
{
public:
	NXNullDescriptor();
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetNullSRV() const { return m_nullSRV; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetNullUAV() const { return m_nullUAV; }
	void ExecuteTasks();

private:
	std::unique_ptr<DescriptorAllocator<false>>	m_pNullAllocator;

	D3D12_CPU_DESCRIPTOR_HANDLE m_nullUAV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_nullSRV;
};
