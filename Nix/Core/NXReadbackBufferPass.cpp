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
		// 从（一般是主渲染cmdList）将RT拷到readback ringbuffer（ctx.pResource）
		pCmdList->CopyBufferRegion(ctx.pResource, ctx.pResourceOffset, pReadbackBuffer->GetD3DResource(), 0, pReadbackBuffer->GetByteSize()); 

		NXReadbackSys->FinishTask(ctx, [ctx]() {
			// 这时候对应的ringBuffer还不会释放 放心用
			uint8_t* pData = ctx.pResourceData + ctx.pResourceOffset;
			});
	}
}
