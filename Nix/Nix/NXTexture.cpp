#include "NXTexture.h"
#include "NXGlobalDefinitions.h"
#include "DirectXTex.h"
#include "NXResourceManager.h"
#include "NXConverter.h"
#include "NXRandom.h"
#include "NXAllocatorManager.h"

NXTexture::NXTexture(NXResourceType type) :
	NXResource(type),
	m_width(-1),
	m_height(-1),
	m_arraySize(-1),
	m_texFormat(DXGI_FORMAT_UNKNOWN),
	m_mipLevels(-1),
	m_useSubRegion(false),
	m_subRegionXY(-1, -1),
	m_subRegionSize(-1, -1),
	m_texFilePath(""),
	m_loadingTexChunks(0),
	m_loadingViews(0),
	m_loading2DPreviews(0),
	m_loadTexChunksReady(true),
	m_loadViewsReady(true),
	m_load2DPreviewsReady(true),
	m_futureLoadingViews(m_promiseLoadingViews.get_future()),
	m_futureLoadingTexChunks(m_promiseLoadingTexChunks.get_future()),
	m_futureLoading2DPreview(m_promiseLoading2DPreview.get_future())
{
}

NXTexture::~NXTexture()
{
}

D3D12_CPU_DESCRIPTOR_HANDLE NXTexture::GetSRV(uint32_t index)
{
	WaitLoadingViewsFinish();
	return m_pSRVs[index];
}

D3D12_CPU_DESCRIPTOR_HANDLE NXTexture::GetRTV(uint32_t index)
{
	WaitLoadingViewsFinish();
	return m_pRTVs[index];
}

D3D12_CPU_DESCRIPTOR_HANDLE NXTexture::GetDSV(uint32_t index)
{
	WaitLoadingViewsFinish();
	return m_pDSVs[index];
}

D3D12_CPU_DESCRIPTOR_HANDLE NXTexture::GetUAV(uint32_t index)
{
	WaitLoadingViewsFinish();
	return m_pUAVs[index];
}

void NXTexture::SetTexChunks(int chunks)
{
	m_loadingTexChunks.store(chunks);
	m_loadTexChunksReady.store(false);
}

void NXTexture::ProcessLoadingTexChunks()
{
	int oldVal = m_loadingTexChunks.fetch_sub(1);

	if (oldVal == 1) // don't use m_loadingTexChunks == 0. It will be fucked up.
	{
		m_promiseLoadingTexChunks.set_value();
		m_loadTexChunksReady.store(true);
	}
}

void NXTexture::SetViews(uint32_t srvNum, uint32_t rtvNum, uint32_t dsvNum, uint32_t uavNum, uint32_t otherNum)
{
	m_pSRVs.resize(srvNum);
	m_pRTVs.resize(rtvNum);
	m_pDSVs.resize(dsvNum);
	m_pUAVs.resize(uavNum);

	m_loadingViews.store(srvNum + rtvNum + dsvNum + uavNum + otherNum);
	m_loadViewsReady.store(false);
}

void NXTexture::ProcessLoadingViews()
{
	int oldVal = m_loadingViews.fetch_sub(1);

	if (oldVal == 1) // don't use m_loadingViews == 0. It will be fucked up.
	{
		m_promiseLoadingViews.set_value();
		m_loadViewsReady.store(true);
	}
}

void NXTexture::WaitLoadingTexturesFinish()
{
	m_futureLoadingTexChunks.wait();
}

void NXTexture::WaitLoadingViewsFinish()
{
	m_futureLoadingViews.wait();
}

void NXTexture::ProcessLoading2DPreview()
{
	int oldVal = m_loading2DPreviews.fetch_sub(1);

	if (oldVal == 1) // don't use m_loadingViews == 0. It will be fucked up.
	{
		m_promiseLoading2DPreview.set_value();
		m_load2DPreviewsReady.store(true);
	}
}

void NXTexture::WaitLoading2DPreviewFinish()
{
	m_futureLoading2DPreview.wait();
}

bool NXTexture::IsLoadReady() const
{
	return m_loadTexChunksReady.load() && m_loadViewsReady.load() && m_load2DPreviewsReady.load();
}

D3D12_CPU_DESCRIPTOR_HANDLE NXTexture::GetSRVPreview(uint32_t index)
{
	if (m_pSRVPreviews.size() > index)
	{
		WaitLoading2DPreviewFinish();
		return m_pSRVPreviews[index];
	}

	if (!m_pSRVs.empty())
	{
		WaitLoadingViewsFinish();
		return m_pSRVs[0];
	}

	return D3D12_CPU_DESCRIPTOR_HANDLE(0);
}

void NXTexture::SetClearValue(float R, float G, float B, float A)
{
	m_clearValue.Color[0] = R;
	m_clearValue.Color[1] = G;
	m_clearValue.Color[2] = B;
	m_clearValue.Color[3] = A;
	m_clearValue.Format = NXConvert::DXGINoTypeless(m_texFormat);
}

void NXTexture::SetClearValue(float depth, uint32_t stencilRef)
{
	m_clearValue.DepthStencil.Depth = depth;
	m_clearValue.DepthStencil.Stencil = stencilRef;
	m_clearValue.Format = NXConvert::DXGINoTypeless(m_texFormat, true);
}

void NXTexture::SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state)
{
	if (m_resourceState == state)
		return;

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pTexture.Get();
	barrier.Transition.StateBefore = m_resourceState;
	barrier.Transition.StateAfter = state;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pCommandList->ResourceBarrier(1, &barrier);

	m_resourceState = state;
}

void NXTexture::CreateRenderTextureInternal(D3D12_RESOURCE_FLAGS flags)
{
	// 创建RT纹理时，没有从文件读取资源的需求，不需要提供上传堆资源。
	// 不走任何Allocator，直接创建资源；ComPtr自己管理资源m_pTexture的生命周期
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = GetResourceDimentionFromType();
	desc.Width = m_width;
	desc.Height = m_height;
	desc.DepthOrArraySize = m_arraySize;
	desc.MipLevels = m_mipLevels;
	desc.Format = m_texFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;

	HRESULT hr;
	if (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		m_resourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		SetClearValue(0.0f, 0.0f, 0.0f, 1.0f);

	}
	else if (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		m_resourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		SetClearValue(1.0f, 0x00);
	}
	else
	{
		m_resourceState = D3D12_RESOURCE_STATE_COMMON;
		SetClearValue(1.0f, 0x00);
	}

	hr = NXGlobalDX::GetDevice()->CreateCommittedResource(
		&NX12Util::CreateHeapProperties(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		m_resourceState,
		&m_clearValue,
		IID_PPV_ARGS(&m_pTexture)
	);

	std::wstring wName(m_name.begin(), m_name.end());
	m_pTexture->SetName(wName.c_str());
	SetRefCountDebugName(m_name);

	if (FAILED(hr))
	{
		m_pTexture.Reset();
		return;
	}

	SetTexChunks(1);
	ProcessLoadingTexChunks();
}

void NXTexture::CreateInternal(const std::shared_ptr<DirectX::ScratchImage>& pImage, D3D12_RESOURCE_FLAGS flags, bool useSubRegion, Int2 subRegionXY, Int2 subRegionSize)
{
	m_useSubRegion = useSubRegion;
	m_subRegionXY = Int2(0, 0);
	m_subRegionSize = Int2(-1, -1);

	DirectX::TexMetadata md = pImage->GetMetadata();
	NXTextureLoaderTaskResult fake{};
	fake.metadata = md;
	fake.pImage = pImage;

	AfterTexLoaded(m_texFilePath, flags, fake);
}

void NXTexture::CreatePathTextureInternal(const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags, bool useSubRegion, Int2 subRegionXY, Int2 subRegionSize)
{
	m_useSubRegion = useSubRegion;
	m_subRegionXY = subRegionXY;
	m_subRegionSize = subRegionSize;

	NXTextureLoaderTask task;
	task.path = filePath;
	task.type = m_type;
	task.serializationData = m_serializationData;
	task.pCallBack = [this, filePath, flags](NXTextureLoaderTaskResult result) {
		AfterTexLoaded(filePath, flags, result);
		};
	NXTexLoader->AddTask(task);
}

void NXTexture::AfterTexLoaded(const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags, const NXTextureLoaderTaskResult& result)
{
	// 获取NXTexLoader的加载结果
	auto& metadata = result.metadata;
	std::shared_ptr<ScratchImage> pImage = result.pImage;

	uint32_t srcW = (uint32_t)metadata.width;
	uint32_t srcH = (uint32_t)metadata.height;
	uint32_t subX = 0, subY = 0, subW = srcW, subH = srcH;
	if (m_useSubRegion)
	{
		// 目前只有2D类型支持subRegion
		if (m_type == NXResourceType::Tex2D)
		{
			subX = m_subRegionXY.x;
			subY = m_subRegionXY.y;
			subW = m_subRegionSize.x;
			subH = m_subRegionSize.y;

			// 如果超过屏幕边界 保留像素
			if (subX >= srcW) subX = srcW - 1;
			if (subY >= srcH) subY = srcH - 1;
			subW = std::min(subW, srcW - subX);
			subH = std::min(subH, srcH - subY);

			// BC 对齐：4x4 block
			if (NXConvert::IsBCFormat(metadata.format))
			{
				// 角坐标按4向下对齐
				uint32_t x0 = AlignDownForPow2Only(subX, 4);
				uint32_t y0 = AlignDownForPow2Only(subY, 4);

				// 子宽高按4向下对齐
				uint32_t x1 = subX + subW;
				uint32_t y1 = subY + subH;
				x1 = AlignDownForPow2Only(x1, 4); 
				y1 = AlignDownForPow2Only(y1, 4); 

				// 防御：if 避免零块
				if (x1 <= x0) x1 = x0 + 4;
				if (y1 <= y0) y1 = y0 + 4;

				// 防御：避免 零块+4后 超出边界
				subX = x0; subY = y0; 
				subW = std::min(srcW - subX, x1 - x0);
				subH = std::min(srcH - subY, y1 - y0);
			}
		}

		m_subRegionXY = Int2(subX, subY);
	}

	// 计算实际使用的desc; 如果有子区域需要调整宽高和mipLevels
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = GetResourceDimentionFromType();
	desc.Width = m_useSubRegion ? subW : srcW;
	desc.Height = m_useSubRegion ? subH : srcH;
	desc.DepthOrArraySize = (uint32_t)metadata.arraySize;
	const uint32_t subMipMax = CalcMipCount((uint32_t)desc.Width, (uint32_t)desc.Height);
	desc.MipLevels = (uint32_t)std::min<uint32_t>(metadata.mipLevels, subMipMax); // NEW
	desc.Format = metadata.format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;

	m_width = (uint32_t)desc.Width;
	m_height = (uint32_t)desc.Height;
	m_arraySize = (uint32_t)desc.DepthOrArraySize;
	m_mipLevels = (uint32_t)desc.MipLevels;
	m_texFormat = desc.Format;
	if (m_useSubRegion)
	{
		m_subRegionSize = Int2(m_width, m_height);
	}

	// desc -> layout
	const uint32_t layoutSize = desc.DepthOrArraySize * desc.MipLevels;
	std::shared_ptr<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]> layouts = std::make_shared<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]>(layoutSize);
	std::shared_ptr<uint32_t[]> numRow = std::make_shared<uint32_t[]>(layoutSize);
	std::shared_ptr<uint64_t[]> numRowSizeInBytes = std::make_shared<uint64_t[]>(layoutSize);
	size_t totalBytes = 0;
	NXGlobalDX::GetDevice()->GetCopyableFootprints(&desc, 0, layoutSize, 0, layouts.get(), numRow.get(), numRowSizeInBytes.get(), &totalBytes);

	// 将加载过程按字节大小拆分成多个 Chunk（内部最大 8MB）
	std::vector<NXTextureUploadChunk> chunks;
	GenerateUploadChunks(layoutSize, numRow.get(), numRowSizeInBytes.get(), totalBytes, chunks);
	SetTexChunks((int)chunks.size());

	NXAllocator_Tex->Alloc(&desc, (uint32_t)totalBytes, [this, result, chunks = std::move(chunks)](const PlacedBufferAllocTaskResult& taskResult) mutable {
		AfterTexMemoryAllocated(result, taskResult, std::move(chunks));
		});
}

void NXTexture::AfterTexMemoryAllocated(const NXTextureLoaderTaskResult& result, const PlacedBufferAllocTaskResult& taskResult, std::vector<NXTextureUploadChunk>&& chunks)
{
	std::shared_ptr<ScratchImage> pImage = result.pImage;

	// 输出资源
	m_pTexture = taskResult.pResource;
	m_pTexture->SetName(NXConvert::s2ws(m_name).c_str());
	SetRefCountDebugName(m_name);
	m_resourceState = D3D12_RESOURCE_STATE_COPY_DEST;

	for (auto& texChunk : chunks)
	{
		if (texChunk.layoutIndexSize == -1)
		{
			CopyPartOfLayoutToChunk(texChunk, pImage);
		}
		else
		{
			CopyMultiLayoutsToChunk(texChunk, pImage);
		}
	}
}

void NXTexture::GenerateUploadChunks(uint32_t layoutSize, uint32_t* numRow, uint64_t* numRowSizeInBytes, uint64_t totalBytes, std::vector<NXTextureUploadChunk>& oChunks)
{
	// 作用：根据纹理布局，将上传任务划分成若干个块上传，避免NXUploadSystem一次性上传过大的数据直接崩溃
	// 思路：按layout遍历
	//	1. layout大小超过了这个阈值，就需要拆分成多个任务。
	//	2. 否则持续累积bytes，每当累积的bytes超过了一个阈值，就生成一个上传任务。

	uint64_t ringBufferLimit = 8 * 1024 * 1024; // 8MB，ringbuffer有64M限制且不允许满载，这里取8M
	for (uint32_t i = 0; i < layoutSize; )
	{
		uint64_t numRowSizeInByteAlign256 = (numRowSizeInBytes[i] + 255) & ~255; // DX12的纹理需要每行按256字节对齐
		uint64_t layoutByteSize = numRow[i] * numRowSizeInByteAlign256;
		if (layoutByteSize > ringBufferLimit)
		{
			// 1. layout大小超过了这个阈值，就需要拆分成多个任务。

			// 根据行数拆分，不要直接算字节，DX需要按行对齐
			uint32_t rowLimit = (uint32_t)(ringBufferLimit / numRowSizeInByteAlign256); // 拆分模式下 每个chunk最多多少行
			for (uint32_t j = 0; j < numRow[i]; j += rowLimit)
			{
				NXTextureUploadChunk chunk = {};
				chunk.layoutIndexStart = i;
				chunk.layoutIndexSize = -1; // 拆分模式下只有一个layout
				chunk.rowStart = j;
				chunk.rowSize = std::min(rowLimit, numRow[i] - j);
				chunk.chunkBytes = chunk.rowSize * (int)numRowSizeInByteAlign256;

				oChunks.push_back(chunk);
			}
			i++;
		}
		else
		{
			// 2. 否则持续累积bytes，每当累积的bytes超过了一个阈值，就生成一个上传任务。
			NXTextureUploadChunk chunk = {};
			chunk.chunkBytes = (int)layoutByteSize;
			chunk.layoutIndexStart = i;
			chunk.layoutIndexSize = 1;
			chunk.rowStart = -1; // 累积模式下只可能包含完整的layout，所以row参数没用
			chunk.rowSize = -1;

			i++;

			while (i < layoutSize)
			{
				uint64_t numRowSizeInByteAlign256 = (numRowSizeInBytes[i] + 255) & ~255; // DX12的纹理需要每行按256字节对齐
				uint64_t layoutByteSize = numRow[i] * numRowSizeInByteAlign256;
				if (chunk.chunkBytes + layoutByteSize >= ringBufferLimit)
				{
					break;
				}

				chunk.chunkBytes += (int)layoutByteSize;
				chunk.layoutIndexSize++;
				i++;
			}

			oChunks.push_back(chunk);
		}
	}
}

uint32_t NXTexture::CalcMipCount(int width, int height)
{
	uint32_t levels = 1;
	int size = std::max(width, height);
	while (size > 1) 
	{
		size >>= 1;
		levels++;
	}
	return levels;
}

void NXTexture::ComputeSubRegionOffsets(const std::shared_ptr<ScratchImage>& pImage, int layoutIndex, uint32_t& oSrcRow, uint32_t& oSrcBytes)
{
	if (!m_useSubRegion)
	{
		oSrcRow = 0;
		oSrcBytes = 0;
		return;
	}

	// oSrcRow：行/块行的起始行数
	// oSrcBytes: 每行/块行的起始偏移量
	int mip, slice;
	NXConvert::GetMipSliceFromLayoutIndex(layoutIndex, m_mipLevels, m_arraySize, mip, slice);

	auto* pImg = pImage->GetImage(mip, slice, 0);
	uint32_t sx = 0, sy = 0;
	//获取 subX subY在mip等级的实际像素偏移量
	sx = m_subRegionXY.x >> mip;
	sy = m_subRegionXY.y >> mip;

	if (!NXConvert::IsBCFormat(m_texFormat)) // 如果不是BC压缩格式
	{
		// 通过行字节数（rowPitch）/图像宽度（width)得出每个像素占多少字节
		uint32_t bytesPerPixel = (pImg->width > 0) ? (uint32_t)(pImg->rowPitch / pImg->width) : 0;

		oSrcBytes = bytesPerPixel * sx; // 获取sub区域中，每行的起始偏移量
		oSrcRow = sy; // 获取sub区域的起始行
	}
	else
	{
		// BC：以块为单位
		
		// 明确这一行的block数量
		const uint32_t blocksX = (pImg->width + 3) / 4;

		// 通过行字节数（rowPitch）/ block数量，明确每个块占多少个字节 
		// bc格式下 rowPitch已经按块对齐并压缩 // 只可能是8或16字节
		const uint32_t blockSize = (blocksX > 0) ? (uint32_t)(pImg->rowPitch / blocksX) : 0;

		// bc模式下，sx sy 已经按4向下对齐
		oSrcBytes = (sx / 4) * blockSize; // 获取sub区域中，每块的起始偏移量
		oSrcRow = (sy / 4); // 获取sub区域中，块的起始偏移量
	}
}

void NXTexture::CopyPartOfLayoutToChunk(const NXTextureUploadChunk& texChunk, const std::shared_ptr<ScratchImage>& pImage)
{
	NXUploadContext taskContext("Upload Texture Task");
	if (!NXUploadSys->BuildTask(texChunk.chunkBytes, taskContext))
	{
		printf("Error: [NXUploadSystem::BuildTask] failed when loading NXTexture2D: %s\n", m_texFilePath.string().c_str());
		return;
	}

	// 该 chunk < 单个 layout：只覆盖 layout 的部分行
	const int index = texChunk.layoutIndexStart;

	int mip = 0, slice = 0;
	NXConvert::GetMipSliceFromLayoutIndex(index, m_mipLevels, m_arraySize, mip, slice);

	const Image* pImg = pImage->GetImage(mip, slice, 0);
	const BYTE* pSrcData = pImg->pixels;

	// 映射上传堆ring buffer
	BYTE* pMapped = taskContext.pResourceData;
	BYTE* pDstBase = pMapped + taskContext.pResourceOffset;

	// 计算源列偏移/起始行（像素行或块行）
	uint32_t srcXBytes = 0, baseRow = 0;
	ComputeSubRegionOffsets(pImage, index, baseRow, srcXBytes);

	// 需要拷贝的行范围（相对目标 layout）
	const uint32_t rowSt = texChunk.rowStart;
	const uint32_t rowEd = texChunk.rowStart + texChunk.rowSize;

	// desc -> layout
	auto desc = m_pTexture->GetDesc();
	const uint32_t layoutSize = desc.DepthOrArraySize * desc.MipLevels;
	std::shared_ptr<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]> layouts = std::make_shared<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]>(layoutSize);
	std::shared_ptr<uint32_t[]> numRow = std::make_shared<uint32_t[]>(layoutSize);
	std::shared_ptr<uint64_t[]> numRowSizeInBytes = std::make_shared<uint64_t[]>(layoutSize);
	size_t totalBytes = 0;
	NXGlobalDX::GetDevice()->GetCopyableFootprints(&desc, 0, layoutSize, 0, layouts.get(), numRow.get(), numRowSizeInBytes.get(), &totalBytes);

	for (uint32_t y = rowSt; y < rowEd; ++y)
	{
		// 源行索引：以 baseRow 为起点
		const uint32_t srcRowIndex = baseRow + y;
		const BYTE* pSrcRow = pSrcData + pImg->rowPitch * srcRowIndex + srcXBytes;
		BYTE* pDstRow = pDstBase + layouts[index].Footprint.RowPitch * (y - rowSt);

		// 每行只拷贝“子矩形宽度”的字节数（由 GetCopyableFootprints 计算得到）	
		memcpy(pDstRow, pSrcRow, numRowSizeInBytes[index]);
	}

	// 提交拷贝：从上传堆片段 → 目标纹理的该 subresource 的部分行
	D3D12_TEXTURE_COPY_LOCATION src = {};
	src.pResource = taskContext.pResource;
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint = layouts[index];
	src.PlacedFootprint.Offset = taskContext.pResourceOffset;
	src.PlacedFootprint.Footprint.Height = texChunk.rowSize; // 仅拷贝部分行

	D3D12_TEXTURE_COPY_LOCATION dst = {};
	dst.pResource = m_pTexture.Get();
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = index;

	const uint32_t dstX = 0;
	const uint32_t dstY = rowSt; // 目标 layout 的起始行
	const uint32_t dstZ = 0;

	D3D12_BOX box = {};
	box.left = 0;
	box.right = src.PlacedFootprint.Footprint.Width; // 子纹理的行宽（texels）
	box.top = 0;
	box.bottom = texChunk.rowSize;                     // 行数
	box.front = 0;
	box.back = 1;

	taskContext.pOwner->pCmdList->CopyTextureRegion(&dst, dstX, dstY, dstZ, &src, &box);

	NXUploadSys->FinishTask(taskContext, [this]() { ProcessLoadingTexChunks(); });
}

void NXTexture::CopyMultiLayoutsToChunk(const NXTextureUploadChunk& texChunk, const std::shared_ptr<ScratchImage>& pImage)
{
	NXUploadContext taskContext("Upload Texture Task");
	if (!NXUploadSys->BuildTask(texChunk.chunkBytes, taskContext))
	{
		printf("Error: [NXUploadSystem::BuildTask] failed when loading NXTexture2D: %s\n", m_texFilePath.string().c_str());
		return;
	}

	// 该 chunk 覆盖多个 layout：每个 layout 拷贝完整行
	for (int index = texChunk.layoutIndexStart; index < texChunk.layoutIndexStart + texChunk.layoutIndexSize; ++index)
	{
		int mip = 0, slice = 0;
		NXConvert::GetMipSliceFromLayoutIndex(index, m_mipLevels, m_arraySize, mip, slice);

		const Image* pImg = pImage->GetImage(mip, slice, 0);
		const BYTE* pSrcData = pImg->pixels;

		// desc -> layout
		auto desc = m_pTexture->GetDesc();
		const uint32_t layoutSize = desc.DepthOrArraySize * desc.MipLevels;
		std::shared_ptr<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]> layouts = std::make_shared<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]>(layoutSize);
		std::shared_ptr<uint32_t[]> numRow = std::make_shared<uint32_t[]>(layoutSize);
		std::shared_ptr<uint64_t[]> numRowSizeInBytes = std::make_shared<uint64_t[]>(layoutSize);
		size_t totalBytes = 0;
		NXGlobalDX::GetDevice()->GetCopyableFootprints(&desc, 0, layoutSize, 0, layouts.get(), numRow.get(), numRowSizeInBytes.get(), &totalBytes);

		BYTE* pMapped = taskContext.pResourceData;
		const uint64_t bytesOffset = taskContext.pResourceOffset + layouts[index].Offset - layouts[texChunk.layoutIndexStart].Offset;
		BYTE* pDstBase = pMapped + bytesOffset;

		uint32_t srcXBytes = 0, baseRow = 0;
		ComputeSubRegionOffsets(pImage, index, baseRow, srcXBytes);

		for (uint32_t y = 0; y < numRow[index]; ++y)
		{
			const uint32_t srcRowIndex = baseRow + y;
			const BYTE* pSrcRow = pSrcData + pImg->rowPitch * srcRowIndex + srcXBytes;
			BYTE* pDstRow = pDstBase + layouts[index].Footprint.RowPitch * y;

			memcpy(pDstRow, pSrcRow, numRowSizeInBytes[index]);
		}

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = taskContext.pResource;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = layouts[index];
		src.PlacedFootprint.Offset = bytesOffset;

		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = m_pTexture.Get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = index;

		const uint32_t dstX = 0, dstY = 0, dstZ = 0;
		taskContext.pOwner->pCmdList->CopyTextureRegion(&dst, dstX, dstY, dstZ, &src, nullptr);
	}

	NXUploadSys->FinishTask(taskContext, [this]() { ProcessLoadingTexChunks(); });
}

void NXTexture::InternalReload(Ntr<NXTexture> pReloadTexture)
{
	m_pTexture = pReloadTexture->m_pTexture;

	auto& pReloadSRVs = pReloadTexture->m_pSRVs;
	m_pSRVs.resize(pReloadSRVs.size());

	auto& pReloadDSVs = pReloadTexture->m_pDSVs;
	m_pDSVs.resize(pReloadDSVs.size());

	auto& pReloadRTVs = pReloadTexture->m_pRTVs;
	m_pRTVs.resize(pReloadRTVs.size());

	auto& pReloadUAVs = pReloadTexture->m_pUAVs;
	m_pUAVs.resize(pReloadUAVs.size());

	for (size_t i = 0; i < pReloadSRVs.size(); i++) m_pSRVs[i] = pReloadSRVs[i];
	for (size_t i = 0; i < pReloadDSVs.size(); i++) m_pDSVs[i] = pReloadDSVs[i];
	for (size_t i = 0; i < pReloadRTVs.size(); i++) m_pRTVs[i] = pReloadRTVs[i];
	for (size_t i = 0; i < pReloadUAVs.size(); i++) m_pUAVs[i] = pReloadUAVs[i];
}

D3D12_RESOURCE_DIMENSION NXTexture::GetResourceDimentionFromType()
{
	switch (m_type)
	{
	case NXResourceType::None: 
		return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	case NXResourceType::Tex1D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	case NXResourceType::Tex2D:
	case NXResourceType::TexCube:
	case NXResourceType::Tex2DArray:
		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	case NXResourceType::Tex3D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	default:
		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	}
}

void NXTexture::Release()
{
}

void NXTexture::MarkReload(const std::filesystem::path& newTexPath)
{
	if (m_reload.m_isReloading)
		return;

	m_reload.m_needReload = true;
	m_reload.m_newTexPath = newTexPath;
}

void NXTexture::ReloadCheck()
{
	if (!m_reload.m_needReload)
		return;

	// 即时替换成过渡纹理，并开始异步加载新纹理
	// 过渡纹理这时肯定加载好了，不需要调用Wait相关方法.
	m_reload.m_isReloading = true;
	m_reload.m_needReload = false;
	InternalReload(NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_White));

	// 异步加载新纹理
	m_reload.m_pReloadTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(m_name, m_reload.m_newTexPath, true);
	std::thread([this]() mutable {
		auto& pNewTex = m_reload.m_pReloadTex;
		pNewTex->WaitLoadingTexturesFinish();
		pNewTex->WaitLoadingViewsFinish();
		m_reload.m_isReloading = false;
		InternalReload(pNewTex);
	}).detach();
}

void NXTexture::Serialize()
{
	using namespace rapidjson;

	if (m_texFilePath.empty())
	{
		printf("Warning, %s couldn't be serialized, cause path %s does not exist.\n", m_texFilePath.string().c_str(), m_texFilePath.string().c_str());
		return;
	}

	NXSerializer serializer;
	std::string nxInfoPath = m_texFilePath.string() + ".n0";

	std::string str = m_texFilePath.extension().string();
	if (NXConvert::IsImageFileExtension(str))
	{
		// 2023.5.30 纹理资源的序列化: 
		serializer.StartObject();
		serializer.String("NXInfoPath", nxInfoPath);	// 元文件路径
		serializer.Uint64("PathHashValue", std::filesystem::hash_value(m_texFilePath)); // 纹理文件路径 hash value
		serializer.Int("TextureType", (int)m_serializationData.m_textureType); // 纹理类型
		serializer.Bool("IsInvertNormalY", m_serializationData.m_bInvertNormalY); // 是否FlipY法线
		serializer.Bool("IsGenerateMipMap", m_serializationData.m_bGenerateMipMap); // 是否生成mipmap
		serializer.Bool("IsCubeMap", m_serializationData.m_bCubeMap); // 是否是立方体贴图
		serializer.EndObject();
	}
	else if (NXConvert::IsRawFileExtension(str))
	{
		serializer.StartObject();
		serializer.String("NXInfoPath", nxInfoPath);	// 元文件路径
		serializer.Uint64("PathHashValue", std::filesystem::hash_value(m_texFilePath)); // 纹理文件路径 hash value
		serializer.Uint("rawFile_Width", m_serializationData.m_rawWidth);
		serializer.Uint("rawFile_Height", m_serializationData.m_rawHeight);
		serializer.Uint("rawFile_ByteSize", m_serializationData.m_rawByteSize);
		serializer.EndObject();
	}

	serializer.SaveToFile(nxInfoPath.c_str());
}

void NXTexture::Deserialize()
{
	using namespace rapidjson;
	std::string nxInfoPath = m_texFilePath.string() + ".n0";
	NXDeserializer deserializer;
	bool bJsonExist = deserializer.LoadFromFile(nxInfoPath.c_str());
	if (bJsonExist)
	{
		// 优先读取序列化文件
		std::string str = m_texFilePath.extension().string();
		if (NXConvert::IsImageFileExtension(str))
		{
			//std::string strPathInfo;
			//strPathInfo = deserializer.String("NXInfoPath");
			m_serializationData.m_textureType = (NXTextureMode)deserializer.Int("TextureType");
			m_serializationData.m_bInvertNormalY = deserializer.Bool("IsInvertNormalY");
			m_serializationData.m_bGenerateMipMap = deserializer.Bool("IsGenerateMipMap");
			m_serializationData.m_bCubeMap = deserializer.Bool("IsCubeMap");
		}
		else if (NXConvert::IsRawFileExtension(str))
		{
			m_serializationData.m_rawWidth = deserializer.Uint("rawFile_Width");
			m_serializationData.m_rawHeight = deserializer.Uint("rawFile_Height");
			m_serializationData.m_rawByteSize = deserializer.Uint("rawFile_ByteSize");
		}
	}
	else if (NXConvert::IsDDSFileExtension(m_texFilePath.extension().string()))
	{
		// 如果没有序列化文件，但是DDS文件，直接读取DDS文件的元数据，这样可以序列化一部分内容
		DirectX::TexMetadata metadata;
		HRESULT hr = DirectX::GetMetadataFromDDSFile(m_texFilePath.c_str(), DirectX::DDS_FLAGS_NONE, metadata);
		if (SUCCEEDED(hr))
		{
			m_serializationData.m_bGenerateMipMap = metadata.mipLevels > 1;
			m_serializationData.m_bCubeMap = metadata.IsCubemap();
		}
	}
	else 
	{
		// 其他情况无法序列化
	}
}

Ntr<NXTexture2D> NXTexture2D::Create(const std::string& debugName, const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags, bool useSubRegion, const Int2& subRegionXY, const Int2& subRegionSize)
{
	m_texFilePath = filePath;
	m_name = debugName;
	Deserialize();

	CreatePathTextureInternal(m_texFilePath, flags, useSubRegion, subRegionXY, subRegionSize);
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateRenderTexture(const std::string& debugName, DXGI_FORMAT fmt, uint32_t width, uint32_t height, D3D12_RESOURCE_FLAGS flags)
{
	m_texFilePath = "[Render Target: " + debugName + "]";
	m_name = debugName;
	m_width = width;
	m_height = height;
	m_arraySize = 1;
	m_mipLevels = 1;
	m_texFormat = fmt;

	CreateRenderTextureInternal(flags);

	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateSolid(const std::string& debugName, uint32_t texSize, const Vector4& color, D3D12_RESOURCE_FLAGS flags)
{
	// 创建大小为 texSize * texSize 的纯色纹理
	std::shared_ptr<ScratchImage> pImage = std::make_shared<ScratchImage>();
	DXGI_FORMAT fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
	pImage->Initialize2D(fmt, texSize, texSize, 1, 1);

	// Convert floating point color to 8-bit color
	uint8_t r = static_cast<uint8_t>(color.x * 255.0f);
	uint8_t g = static_cast<uint8_t>(color.y * 255.0f);
	uint8_t b = static_cast<uint8_t>(color.z * 255.0f);
	uint8_t a = static_cast<uint8_t>(color.w * 255.0f);

	const Image& pImg = *pImage->GetImage(0, 0, 0);
	for (uint32_t i = 0; i < texSize; ++i)
	{
		for (uint32_t j = 0; j < texSize; ++j)
		{
			uint8_t* pixel = pImg.pixels + i * pImg.rowPitch + j * 4;
			pixel[0] = r;
			pixel[1] = g;
			pixel[2] = b;
			pixel[3] = a;
		}
	}

	m_texFilePath = "[Default Solid Texture]";
	m_name = debugName;

	CreateInternal(pImage, flags, false, Int2(0, 0), Int2(-1, -1));

	pImage.reset();
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateNoise(const std::string& debugName, uint32_t texSize, uint32_t dimension, D3D12_RESOURCE_FLAGS flags)
{
	// 创建大小为 texSize * texSize 的噪声纹理

	// Check if dimension is valid (1, 2, 3, or 4)
	if (dimension < 1 || dimension > 4)
	{
		printf("Invalid Dimension for Noise Texture. Allowed values are 1, 2, 3, or 4.\n");
		return nullptr;
	}

	DXGI_FORMAT fmt;
	switch (dimension)
	{
	case 1: fmt = DXGI_FORMAT_R32_FLOAT; break;
	case 2: fmt = DXGI_FORMAT_R32G32_FLOAT; break;
	case 3: fmt = DXGI_FORMAT_R32G32B32_FLOAT; break;
	case 4: fmt = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
	}

	uint32_t bytePerPixel = sizeof(float) * dimension; // 2023.10.26 现阶段只支持 32-bit float 纹理，所以每个像素占 sizeof(float) * Dimension 字节

	std::shared_ptr<ScratchImage> pImage = std::make_shared<ScratchImage>();
	pImage->Initialize2D(fmt, texSize, texSize, 1, 1);

	NXRandom* randInst = NXRandom::GetInstance();
	const Image& image = *pImage->GetImage(0, 0, 0);
	for (uint32_t i = 0; i < texSize; ++i)
	{
		for (uint32_t j = 0; j < texSize; ++j)
		{
			float* pixel = reinterpret_cast<float*>(image.pixels + i * image.rowPitch + j * dimension * bytePerPixel);
			for(uint32_t dim = 0; dim < dimension; dim++)
				pixel[dim] = randInst->CreateFloat();
		}
	}

	m_texFilePath = "[Default Noise Texture]";
	m_name = debugName;

	CreateInternal(pImage, flags, false, Int2(0, 0), Int2(-1, -1));

	pImage.reset();
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateHeightRaw(const std::string& debugName, const std::filesystem::path& rawPath, D3D12_RESOURCE_FLAGS flags, bool useSubRegion, const Int2& subRegionXY, const Int2& subRegionSize)
{
	m_texFilePath = rawPath;
	Deserialize();

	int width = m_serializationData.m_rawWidth;
	int height = m_serializationData.m_rawHeight;
	if (useSubRegion)
	{
		width = subRegionSize.x;
		height = subRegionSize.y;
	}

	std::vector<uint16_t> rawData(width * height);

	// 读取rawPath的文件，并转换成单通道纹理
	std::ifstream file(rawPath, std::ios::binary);
	if (!file)
		throw std::runtime_error("无法打开文件: " + rawPath.string());

	// 直接读数据就行，必须是16bit 
	// todo: 支持更多格式
	file.read(reinterpret_cast<char*>(rawData.data()), width * height * sizeof(uint16_t));

	if (!file)
		throw std::runtime_error("读取数据失败: " + rawPath.string());

	uint32_t bytePerPixel = sizeof(uint16_t);

	auto fmt = DXGI_FORMAT_R16_UNORM;
	std::shared_ptr<ScratchImage> pImage = std::make_shared<ScratchImage>();
	pImage->Initialize2D(fmt, width, height, 1, 1);

	const Image& image = *pImage->GetImage(0, 0, 0);
	uint8_t* p = image.pixels;

	bool yFlip = false;
	if (yFlip)
	{
		for (int y = 0; y < height; ++y)
		{
			uint8_t* srcRow = (uint8_t*)rawData.data() + (height - 1 - y) * width * bytePerPixel;
			uint8_t* dstRow = p + y * image.rowPitch;
			memcpy(dstRow, srcRow, width * bytePerPixel);
		}
	}
	else memcpy(p, rawData.data(), width * height * bytePerPixel);

	m_name = debugName;

	CreateInternal(pImage, flags, useSubRegion, subRegionXY, subRegionSize);

	pImage.reset();
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateByData(const std::string& debugName, const std::shared_ptr<ScratchImage>& pImage, D3D12_RESOURCE_FLAGS flags)
{
	m_name = debugName;
	CreateInternal(pImage, flags, false, Int2(0, 0), Int2(-1, -1));
	return this;
}

void NXTexture2D::SetSRV(uint32_t index)
{
	NXAllocator_SRV->Alloc([this, index](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pSRVs[index] = result;
		WaitLoadingTexturesFinish(); // 创建SRV前，先等待纹理加载完成

		DXGI_FORMAT SRVFormat = m_texFormat;
		if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
			SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 默认的映射
		srvDesc.Format = SRVFormat; // 纹理的格式
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = m_mipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0;
		srvDesc.Texture2D.PlaneSlice = 0;

		NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pSRVs[index]);

		ProcessLoadingViews();
		});
}

void NXTexture2D::SetRTV(uint32_t index)
{
	NXAllocator_RTV->Alloc([this, index](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pRTVs[index] = result;
		NXGlobalDX::GetDevice()->CreateRenderTargetView(m_pTexture.Get(), nullptr, m_pRTVs[index]);
		ProcessLoadingViews();
		});
}

void NXTexture2D::SetDSV(uint32_t index)
{
	NXAllocator_DSV->Alloc([this, index](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pDSVs[index] = result;

		DXGI_FORMAT DSVFormat = m_texFormat;
		if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
			DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		else if (m_texFormat == DXGI_FORMAT_R32_TYPELESS)
			DSVFormat = DXGI_FORMAT_D32_FLOAT;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DSVFormat;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		NXGlobalDX::GetDevice()->CreateDepthStencilView(m_pTexture.Get(), &dsvDesc, m_pDSVs[index]);

		ProcessLoadingViews();
		});
}

void NXTexture2D::SetUAV(uint32_t index)
{
	NXAllocator_SRV->Alloc([this, index](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pUAVs[index] = result;
		NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pTexture.Get(), nullptr, nullptr, m_pUAVs[index]);
		ProcessLoadingViews();
		});
}

void NXTextureCube::Create(const std::string& debugName, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags)
{
	m_name = debugName;
	m_width = width;
	m_height = height;
	m_arraySize = 6; 	// textureCube must be 6.
	m_texFormat = texFormat;
	m_mipLevels = mipLevels;

	CreateRenderTextureInternal(flags);

	SetSRVPreviews();
}

void NXTextureCube::Create(const std::string& debugName, const std::wstring& filePath, D3D12_RESOURCE_FLAGS flags)
{
	m_texFilePath = filePath.c_str();
	m_name = debugName;

	CreatePathTextureInternal(m_texFilePath, flags, false, Int2(0, 0), Int2(-1, -1));

	SetSRVPreviews();
}

void NXTextureCube::SetSRV(uint32_t index)
{
	NXAllocator_SRV->Alloc([this, index](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pSRVs[index] = result;
		WaitLoadingTexturesFinish(); // 创建SRV前，先等待纹理加载完成

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = m_texFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.TextureCube.MipLevels = m_mipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pSRVs[index]);

		ProcessLoadingViews();
		});
}

D3D12_CPU_DESCRIPTOR_HANDLE NXTextureCube::GetSRVPreview(uint32_t index)
{
	WaitLoading2DPreviewFinish();
	return { m_pSRVPreviews[index] };
}

void NXTextureCube::SetSRVPreviews()
{
	m_loading2DPreviews.store(6);
	m_load2DPreviewsReady.store(false);

	m_pSRVPreviews.resize(6);
	for (auto i = 0; i < m_pSRVPreviews.size(); i++)
	{
		SetSRVPreview(i);
	}
}

void NXTextureCube::SetSRVPreview(uint32_t idx)
{
	NXAllocator_SRV->Alloc([this, idx](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pSRVPreviews[idx] = result;
		WaitLoadingTexturesFinish(); // 创建SRV前，先等待纹理加载完成

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = m_texFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY; // Use a 2D texture array view
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.FirstArraySlice = idx;
		srvDesc.Texture2DArray.ArraySize = 1;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;

		NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pSRVPreviews[idx]);

		ProcessLoading2DPreview();
		});
}

void NXTextureCube::SetRTV(uint32_t index, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_RTV->Alloc([this, index, mipSlice, firstArraySlice, arraySize](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pRTVs[index] = result;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = m_texFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = mipSlice;
		rtvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		rtvDesc.Texture2DArray.ArraySize = arraySize;
		rtvDesc.Texture2DArray.PlaneSlice = 0;

		NXGlobalDX::GetDevice()->CreateRenderTargetView(m_pTexture.Get(), &rtvDesc, m_pRTVs[index]);

		ProcessLoadingViews();
		});
}

void NXTextureCube::SetDSV(uint32_t index, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_DSV->Alloc([this, index, mipSlice, firstArraySlice, arraySize](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pDSVs[index] = result;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Format = m_texFormat;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2DArray.MipSlice = mipSlice;
		dsvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		dsvDesc.Texture2DArray.ArraySize = arraySize;

		NXGlobalDX::GetDevice()->CreateDepthStencilView(m_pTexture.Get(), &dsvDesc, m_pDSVs[index]);

		ProcessLoadingViews();
		});
}

void NXTextureCube::SetUAV(uint32_t index, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_SRV->Alloc([this, index, mipSlice, firstArraySlice, arraySize](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pUAVs[index] = result;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = m_texFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.MipSlice = mipSlice;
		uavDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		uavDesc.Texture2DArray.ArraySize = arraySize;
		uavDesc.Texture2DArray.PlaneSlice = 0;

		NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pTexture.Get(), nullptr, &uavDesc, m_pUAVs[index]);

		ProcessLoadingViews();
		});
}

void NXTexture2DArray::Create(const std::string& debugName, const std::wstring& filePath, D3D12_RESOURCE_FLAGS flags)
{
	m_texFilePath = filePath.c_str();
	m_name = debugName;
	Deserialize();

	CreatePathTextureInternal(m_texFilePath, flags, false, Int2(0, 0), Int2(-1, -1));
	SetSRVPreviews();
}

void NXTexture2DArray::Create(const std::string& debugName, const std::wstring& filePath, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags)
{
	m_texFilePath = filePath.c_str();
	m_name = debugName;
	m_width = width;
	m_height = height;
	m_arraySize = arraySize;
	m_texFormat = texFormat;
	m_mipLevels = mipLevels;
	Deserialize();

	CreatePathTextureInternal(m_texFilePath, flags, false, Int2(0, 0), Int2(-1, -1));
	SetSRVPreviews();
}

void NXTexture2DArray::CreateRT(const std::string& debugName, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags)
{
	m_name = debugName;
	m_width = width;
	m_height = height;
	m_arraySize = arraySize;
	m_texFormat = texFormat;
	m_mipLevels = mipLevels;
	CreateRenderTextureInternal(flags);
}

void NXTexture2DArray::SetSRV(uint32_t index, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_SRV->Alloc([this, index, firstArraySlice, arraySize](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pSRVs[index] = result;
		WaitLoadingTexturesFinish(); // 创建SRV前，先等待纹理加载完成

		DXGI_FORMAT SRVFormat = m_texFormat;
		if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
			SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		else if (m_texFormat == DXGI_FORMAT_R32_TYPELESS)
			SRVFormat = DXGI_FORMAT_R32_FLOAT;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 默认的映射
		srvDesc.Format = SRVFormat; // 纹理的格式
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MipLevels = m_mipLevels;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		srvDesc.Texture2DArray.ArraySize = arraySize;

		NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pSRVs[index]);

		ProcessLoadingViews();
		});
}

void NXTexture2DArray::SetRTV(uint32_t index, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_RTV->Alloc([this, index, firstArraySlice, arraySize](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pRTVs[index] = result;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = m_texFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		rtvDesc.Texture2DArray.ArraySize = arraySize;
		rtvDesc.Texture2DArray.PlaneSlice = 0;

		NXGlobalDX::GetDevice()->CreateRenderTargetView(m_pTexture.Get(), &rtvDesc, m_pRTVs[index]);

		ProcessLoadingViews();
		});
}

void NXTexture2DArray::SetDSV(uint32_t index, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_DSV->Alloc([this, index, firstArraySlice, arraySize](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pDSVs[index] = result;

		DXGI_FORMAT DSVFormat = m_texFormat;
		if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
			DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		else if (m_texFormat == DXGI_FORMAT_R32_TYPELESS)
			DSVFormat = DXGI_FORMAT_D32_FLOAT;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DSVFormat;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		dsvDesc.Texture2DArray.ArraySize = arraySize;

		NXGlobalDX::GetDevice()->CreateDepthStencilView(m_pTexture.Get(), &dsvDesc, m_pDSVs[index]);

		ProcessLoadingViews();
		});
}

void NXTexture2DArray::SetUAV(uint32_t index, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_SRV->Alloc([this, index, firstArraySlice, arraySize](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pUAVs[index] = result;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = m_texFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.MipSlice = 0;
		uavDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		uavDesc.Texture2DArray.ArraySize = arraySize;

		NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pTexture.Get(), nullptr, &uavDesc, m_pUAVs[index]);

		ProcessLoadingViews();
		});
}

D3D12_CPU_DESCRIPTOR_HANDLE NXTexture2DArray::GetSRVPreview(uint32_t index)
{
	WaitLoading2DPreviewFinish();
	return { m_pSRVPreviews[index] };
}

void NXTexture2DArray::SetSRVPreviews()
{
	DirectX::TexMetadata metadata;
	HRESULT hr = DirectX::GetMetadataFromDDSFile(m_texFilePath.c_str(), DirectX::DDS_FLAGS_NONE, metadata);

	// 这里不能直接用m_arraySize，m_arraySize在纹理异步加载完成时才有效
	m_loading2DPreviews = metadata.arraySize;
	m_load2DPreviewsReady.store(false);

	m_pSRVPreviews.resize(metadata.arraySize);
	for (auto i = 0; i < m_pSRVPreviews.size(); i++)
	{
		SetSRVPreview(i);
	}
}

void NXTexture2DArray::SetSRVPreview(uint32_t idx)
{
	NXAllocator_SRV->Alloc([this, idx](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pSRVPreviews[idx] = result;
		WaitLoadingTexturesFinish(); // 创建SRV前，先等待纹理加载完成

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = m_texFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.FirstArraySlice = idx;
		srvDesc.Texture2DArray.ArraySize = 1;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;

		NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pSRVPreviews[idx]);

		ProcessLoading2DPreview();
		});
}
