#include "NXTerrainCommandSignature.h"
#include "NXGlobalDefinitions.h"

NXTerrainCommandSignature::NXTerrainCommandSignature()
{
}

void NXTerrainCommandSignature::Init()
{
	// 创建一个GPU-Driven地形专用的CommandSignatureDesc
	m_drawIndexArgDesc[0] = {};
	m_drawIndexArgDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	
	m_cmdSigDesc = {};
	m_cmdSigDesc.NumArgumentDescs = 1;
	m_cmdSigDesc.pArgumentDescs = m_drawIndexArgDesc;
	m_cmdSigDesc.ByteStride = sizeof(int) * 5;
	m_cmdSigDesc.NodeMask = 0;
}
