#pragma once
#include "BaseDefs/DX12.h"

// 仅负责记录根参数的布局信息
struct NXRGRootParamLayout
{
	// 常量缓冲区数量
	int cbvCount = 0;

	// []内每个元素表示各自space下的资源数量，比如 srvCount[0] 表示 space0 下的 SRV 数量
	std::vector<int> srvCount = {};
	std::vector<int> uavCount = {};
};

struct NXRGCommandContext
{
	MultiFrame<ComPtr<ID3D12CommandAllocator>>		cmdAllocator;
	MultiFrame<ComPtr<ID3D12GraphicsCommandList>>	cmdList;
};
