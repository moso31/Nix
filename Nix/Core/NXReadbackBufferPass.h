#pragma once
#include "NXBuffer.h"

// TODO: ȱ���������� NXReadback��Texture��Pass ��֧��
class NXReadbackBufferPass //: public NXRenderPass
{
public:
	NXReadbackBufferPass();
	virtual ~NXReadbackBufferPass() {}

	virtual void SetupInternal() = 0;
};