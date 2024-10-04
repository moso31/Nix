#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

#include <set>
#include <functional>
#include <vector>
#include <string>

namespace ccmem
{
	struct NonVisibleDescriptorTaskResult
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	};

	struct ShaderVisibleDescriptorTaskResult
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	};
}
