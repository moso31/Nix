#pragma once
#include "BaseDefs/DX12.h"

struct NXRGRootParamLayout
{
	int cbvCount = 0;
	int srvCount = 0;
	int uavCount = 0;
};

struct NXRGCommandContext
{
	MultiFrame<ComPtr<ID3D12CommandAllocator>>		cmdAllocator;
	MultiFrame<ComPtr<ID3D12GraphicsCommandList>>	cmdList;
};
