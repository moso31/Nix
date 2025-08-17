#include "NXReadbackBufferPass.h"
#include "NXAllocatorManager.h"

NXReadbackBufferPass::NXReadbackBufferPass() :
	NXRGPass(NXRenderPassType::ReadbackBufferPass)
{
}

void NXReadbackBufferPass::Render(ID3D12GraphicsCommandList* pCmdList)
{
	auto pReadbackBuffer = m_pReadbackBuffer->GetBuffer();
	NXReadbackContext ctx(pReadbackBuffer->GetName() + "_Buffer");
	if (NXReadbackSys->BuildTask(pReadbackBuffer->GetByteSize(), ctx))
	{
		// �ӣ�һ��������ȾcmdList����RT����readback ringbuffer��ctx.pResource��
		pCmdList->CopyBufferRegion(ctx.pResource, ctx.pResourceOffset, pReadbackBuffer->GetD3DResource(), 0, pReadbackBuffer->GetByteSize()); 

		NXReadbackSys->FinishTask(ctx, [ctx]() {
			// ��ʱ���Ӧ��ringBuffer�������ͷ� ������
			uint8_t* pData = ctx.pResourceData + ctx.pResourceOffset;
			});
	}
}
