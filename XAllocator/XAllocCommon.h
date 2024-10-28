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
#include <mutex>

namespace ccmem
{
	// 着色器不可见的描述符，如一般情况下的 rtv dsv
	struct XDescriptor
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	};

	// 着色器可见的描述符，如 一般情况下的 srv（目前还没什么用得到cbv uav的地方）
	struct XShaderDescriptor
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	};
}
