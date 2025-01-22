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
	// TODO：cbData按Object和View分可以，但现在的分法有问题。
	// cbObject中存在一些view proj之类的参数，实际上应该放在Camera中。 将来再改，现在没空
	ConstantBufferObject						cbDataObject;
	NXConstantBuffer<ConstantBufferObject>		cbObject;
	ConstantBufferCamera						cbDataCamera;
	NXConstantBuffer<ConstantBufferCamera>		cbCamera;
	ConstantBufferShadowTest					cbDataShadowTest;
	NXConstantBuffer<ConstantBufferShadowTest>	cbShadowTest;
};
