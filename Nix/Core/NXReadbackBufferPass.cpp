#include "NXReadbackBufferPass.h"
#include "NXAllocatorManager.h"

NXReadbackBufferPass::NXReadbackBufferPass() :
	NXRGPass(NXRenderPassType::ReadbackBufferPass)
{
}

void NXReadbackBufferPass::Render()
{
	// 在这里维护CPUData（m_pOutData）的大小
	auto pGPUBuffer = m_pReadbackBuffer->GetBuffer();
	auto stride = pGPUBuffer->GetStride();
	auto byteSize = pGPUBuffer->GetByteSize();
	if (m_pOutData->GetStride() != stride || m_pOutData->GetByteSize() != byteSize)
		m_pOutData->Create(stride, byteSize / stride);

	auto pGPUBuffer = m_pReadbackBuffer->GetBuffer();
	NXReadbackContext ctx(pGPUBuffer->GetName() + "_Buffer");
	if (NXReadbackSys->BuildTask(pGPUBuffer->GetByteSize(), ctx))
	{
		// 从（一般是主渲染cmdList）将RT拷到readback ringbuffer（ctx.pResource）
		auto pCmdList = m_commandCtx.cmdList.Current();
		pCmdList->CopyBufferRegion(ctx.pResource, ctx.pResourceOffset, pGPUBuffer->GetD3DResource(), 0, pGPUBuffer->GetByteSize()); 

		NXReadbackSys->FinishTask(ctx, [this, ctx]() {
			// 这时候对应的ringBuffer还不会释放 放心用
			uint8_t* pData = ctx.pResourceData + ctx.pResourceOffset;
			m_pOutData->CopyDataFromGPU(pData);
			});
	}
}
