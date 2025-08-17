#include "NXReadbackBufferPass.h"
#include "NXAllocatorManager.h"

NXReadbackBufferPass::NXReadbackBufferPass() :
	NXRGPass(NXRenderPassType::ReadbackBufferPass)
{
}

void NXReadbackBufferPass::Render(ID3D12GraphicsCommandList* pCmdList)
{
	auto pReadbackBuffer = m_pReadbackBuffer->GetBuffer();
	NXTransferContext ctx(pReadbackBuffer->GetName() + "_Buffer");
	if (NXGPUTransferSys->BuildTask(pReadbackBuffer->GetByteSize(), NXTransferType::Readback, ctx))
	{
		// �ӣ�һ��������ȾcmdList����RT����readback ringbuffer��ctx.pResource��
		pCmdList->CopyBufferRegion(ctx.pResource, ctx.pResourceOffset, pReadbackBuffer->GetD3DResource(), 0, pReadbackBuffer->GetByteSize()); 

		NXGPUTransferSys->FinishTask(ctx, [&]() {
			// ��ʱ���Ӧ��ringBuffer�������ͷ� ������
			uint8_t* pData = ctx.pResourceData + ctx.pResourceOffset;
			});
	}
}
