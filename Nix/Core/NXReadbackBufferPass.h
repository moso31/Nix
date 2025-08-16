#pragma once
#include "NXBuffer.h"

// TODO: 缺少纹理类型 NXReadback（Texture）Pass 的支持
class NXReadbackBufferPass //: public NXRenderPass
{
public:
	NXReadbackBufferPass();
	virtual ~NXReadbackBufferPass() {}

	virtual void SetupInternal() = 0;
};