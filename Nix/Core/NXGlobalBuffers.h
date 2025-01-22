#pragma once
#include "NXInstance.h"
#include "ShaderStructures.h"
#include "NXConstantBuffer.h"

#define g_cbDataObject 		NXGlobalBuffer::GetInstance()->cbDataObject
#define g_cbObject			NXGlobalBuffer::GetInstance()->cbObject
#define g_cbDataCamera 		NXGlobalBuffer::GetInstance()->cbDataCamera
#define g_cbCamera			NXGlobalBuffer::GetInstance()->cbCamera
#define g_cbDataShadowTest 	NXGlobalBuffer::GetInstance()->cbDataShadowTest
#define g_cbShadowTest		NXGlobalBuffer::GetInstance()->cbShadowTest

class NXGlobalBuffer : public NXInstance<NXGlobalBuffer>
{
public:
	// global buffers 2024.11.13 
	// TODO��cbData��Object��View�ֿ��ԣ������ڵķַ������⡣
	// cbObject�д���һЩview proj֮��Ĳ�����ʵ����Ӧ�÷���Camera�С� �����ٸģ�����û��
	ConstantBufferObject						cbDataObject;
	NXConstantBuffer<ConstantBufferObject>		cbObject;
	ConstantBufferCamera						cbDataCamera;
	NXConstantBuffer<ConstantBufferCamera>		cbCamera;
	ConstantBufferShadowTest					cbDataShadowTest;
	NXConstantBuffer<ConstantBufferShadowTest>	cbShadowTest;
};
