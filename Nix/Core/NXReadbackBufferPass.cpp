#include "NXReadbackBufferPass.h"
#include "NXAllocatorManager.h"

NXReadbackBufferPass::NXReadbackBufferPass() :
	NXRGPass(NXRenderPassType::ReadbackBufferPass)
{
}

void NXReadbackBufferPass::Render()
{
	// ������ά��CPUData��m_pOutData���Ĵ�С
	AdjustOutputDataSize();

	auto pGPUBuffer = m_pReadbackBuffer->GetBuffer();
	NXReadbackContext ctx(pGPUBuffer->GetName() + "_Buffer");
	if (NXReadbackSys->BuildTask(pGPUBuffer->GetByteSize(), ctx))
	{
		// �ӣ�һ��������ȾcmdList����RT����readback ringbuffer��ctx.pResource��
		auto pCmdList = m_commandCtx.cmdList.Current();
		pCmdList->CopyBufferRegion(ctx.pResource, ctx.pResourceOffset, pGPUBuffer->GetD3DResource(), 0, pGPUBuffer->GetByteSize()); 

		NXReadbackSys->FinishTask(ctx, [this, ctx]() {
			// ��ʱ���Ӧ��ringBuffer�������ͷ� ������
			uint8_t* pData = ctx.pResourceData + ctx.pResourceOffset;
			m_pOutData->CopyDataFromGPU(pData);
			});
	}
}

void NXReadbackBufferPass::AdjustOutputDataSize()
{
	auto pGPUBuffer = m_pReadbackBuffer->GetBuffer();
	auto stride = pGPUBuffer->GetStride();
	auto byteSize = pGPUBuffer->GetByteSize();

	if (m_pOutData->GetStride() != stride || m_pOutData->GetByteSize() != byteSize)
	{
		m_pOutData->Create(stride, byteSize / stride);
	}
}
