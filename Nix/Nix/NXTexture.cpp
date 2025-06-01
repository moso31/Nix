#include "NXTexture.h"
#include "NXGlobalDefinitions.h"
#include "DirectXTex.h"
#include "NXResourceManager.h"
#include "NXConverter.h"
#include "NXRandom.h"
#include "NXAllocatorManager.h"

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
	m_loadingTexChunks = chunks;
}

void NXTexture::ProcessLoadingTexChunks()
{
	int oldVal = m_loadingTexChunks.fetch_sub(1);

	if (oldVal == 1) // don't use m_loadingTexChunks == 0. It will be fucked up.
	{
		m_promiseLoadingTexChunks.set_value();
	}
}

void NXTexture::SetViews(uint32_t srvNum, uint32_t rtvNum, uint32_t dsvNum, uint32_t uavNum, uint32_t otherNum)
{
	m_pSRVs.resize(srvNum);
	m_pRTVs.resize(rtvNum);
	m_pDSVs.resize(dsvNum);
	m_pUAVs.resize(uavNum);

	m_loadingViews = srvNum + rtvNum + dsvNum + uavNum + otherNum;
}

void NXTexture::ProcessLoadingBuffers()
{
	int oldVal = m_loadingViews.fetch_sub(1);

	if (oldVal == 1) // don't use m_loadingViews == 0. It will be fucked up.
	{
		m_promiseLoadingViews.set_value();
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
	}
}

void NXTexture::WaitLoading2DPreviewFinish()
{
	m_futureLoading2DPreview.wait();
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

void NXTexture::CreateInternal(const std::shared_ptr<DirectX::ScratchImage>& pImage, D3D12_RESOURCE_FLAGS flags)
{
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

	uint32_t layoutSize = desc.DepthOrArraySize * desc.MipLevels;
	std::shared_ptr<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]> layouts = std::make_shared<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]>(layoutSize);
	std::shared_ptr<uint32_t[]> numRow = std::make_shared<uint32_t[]>(layoutSize);
	std::shared_ptr<uint64_t[]> numRowSizeInBytes = std::make_shared<uint64_t[]>(layoutSize);
	size_t totalBytes;
	NXGlobalDX::GetDevice()->GetCopyableFootprints(&desc, 0, layoutSize, 0, layouts.get(), numRow.get(), numRowSizeInBytes.get(), &totalBytes);

	SetTexChunks(1);
	NXAllocator_Tex->Alloc(&desc, (uint32_t)totalBytes, [this, layouts, numRow, numRowSizeInBytes, layoutSize, totalBytes, pImage](const PlacedBufferAllocTaskResult& result) mutable {
		m_pTexture = result.pResource;

		UploadTaskContext taskContext(m_name);
		if (NXUploadSystem->BuildTask((int)totalBytes, taskContext))
		{
			// 更新纹理资源
			m_pTexture->SetName(NXConvert::s2ws(m_name).c_str());
			SetRefCountDebugName(m_name);
			m_resourceState = D3D12_RESOURCE_STATE_COPY_DEST; // 和 NXAllocator_Tex->Alloc 内部的逻辑保持同步

			auto texDesc = m_pTexture->GetDesc();
			for (uint32_t face = 0, index = 0; face < texDesc.DepthOrArraySize; face++)
			{
				for (uint32_t mip = 0; mip < texDesc.MipLevels; mip++, index++)
				{
					const Image* pImg = pImage->GetImage(mip, face, 0);
					const BYTE* pSrcData = pImg->pixels;
					BYTE* pMappedRingBufferData = taskContext.pResourceData;
					uint64_t bytesOffset = taskContext.pResourceOffset + layouts[index].Offset;
					BYTE* pDstData = pMappedRingBufferData + bytesOffset;

					for (uint32_t y = 0; y < numRow[index]; y++)
					{
						memcpy(pDstData + layouts[index].Footprint.RowPitch * y, pSrcData + pImg->rowPitch * y, numRowSizeInBytes[index]);
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

					taskContext.pOwner->pCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
				}
			}

			NXUploadSystem->FinishTask(taskContext, [this]() {
				ProcessLoadingTexChunks();
				});
		}
		});
}

void NXTexture::CreatePathTextureInternal(const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags)
{
	NXTextureLoaderTask task;
	task.path = filePath;
	task.type = m_type;
	task.serializationData = m_serializationData;
	task.pCallBack = [this, flags](NXTextureLoaderTaskResult result) {
		auto& metadata = result.metadata;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = GetResourceDimentionFromType();
		desc.Width = (uint32_t)metadata.width;
		desc.Height = (uint32_t)metadata.height;
		desc.DepthOrArraySize = (uint32_t)metadata.arraySize;
		desc.MipLevels = (uint32_t)metadata.mipLevels;
		desc.Format = metadata.format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = flags;

		uint32_t layoutSize = desc.DepthOrArraySize * desc.MipLevels;
		std::shared_ptr<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]> layouts = std::make_shared<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]>(layoutSize);
		std::shared_ptr<uint32_t[]> numRow = std::make_shared<uint32_t[]>(layoutSize);
		std::shared_ptr<uint64_t[]> numRowSizeInBytes = std::make_shared<uint64_t[]>(layoutSize);
		size_t totalBytes;
		NXGlobalDX::GetDevice()->GetCopyableFootprints(&desc, 0, layoutSize, 0, layouts.get(), numRow.get(), numRowSizeInBytes.get(), &totalBytes);

		std::vector<NXTextureUploadChunk> chunks;
		GenerateUploadChunks(layoutSize, numRow.get(), numRowSizeInBytes.get(), totalBytes, chunks);

		SetTexChunks((int)chunks.size());

		std::filesystem::path filePath = filePath;
		NXAllocator_Tex->Alloc(&desc, (uint32_t)totalBytes, [this, name = m_name, result, filePath, layouts, numRow, numRowSizeInBytes, layoutSize, chunks = std::move(chunks)](const PlacedBufferAllocTaskResult& taskResult) {
			auto& metadata = result.metadata;
			std::shared_ptr<ScratchImage> pImage = result.pImage;

			m_width = (uint32_t)metadata.width;
			m_height = (uint32_t)metadata.height;
			m_arraySize = (uint32_t)metadata.arraySize;
			m_mipLevels = (uint32_t)metadata.mipLevels;
			m_texFormat = metadata.format;
			m_pTexture = taskResult.pResource;

			m_pTexture->SetName(NXConvert::s2ws(name).c_str());
			SetRefCountDebugName(name);
			m_resourceState = D3D12_RESOURCE_STATE_COPY_DEST; // 和 NXAllocator_Tex->Alloc 内部的逻辑保持同步

			for (auto& texChunk : chunks)
			{
				UploadTaskContext taskContext("Upload Texture Task");
				if (NXUploadSystem->BuildTask(texChunk.chunkBytes, taskContext))
				{
					int mip, slice;
					auto texDesc = m_pTexture->GetDesc();

					if (texChunk.layoutIndexSize == -1)
					{
						// chunk里只有单个layout，拆分处理
						int index = texChunk.layoutIndexStart;

						NXConvert::GetMipSliceFromLayoutIndex(index, texDesc.MipLevels, texDesc.DepthOrArraySize, mip, slice);

						const Image* pImg = pImage->GetImage(mip, slice, 0);
						const BYTE* pSrcData = pImg->pixels;
						BYTE* pMappedRingBufferData = taskContext.pResourceData;
						BYTE* pDstData = pMappedRingBufferData + taskContext.pResourceOffset;

						uint32_t rowSt = texChunk.rowStart;
						uint32_t rowEd = texChunk.rowStart + texChunk.rowSize;
						for (uint32_t y = rowSt; y < rowEd; y++)
						{
							memcpy(pDstData + layouts[index].Footprint.RowPitch * (y - rowSt), pSrcData + pImg->rowPitch * y, numRowSizeInBytes[index]);
						}

						// NXUploadSystem 从RingBuffer同步到实际GPU资源
						D3D12_TEXTURE_COPY_LOCATION src = {};
						src.pResource = taskContext.pResource;
						src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
						src.PlacedFootprint = layouts[index];
						src.PlacedFootprint.Footprint.Height = texChunk.rowSize; // src = 上传堆中的内存段，明确chunk对应的大小即可
						src.PlacedFootprint.Offset = taskContext.pResourceOffset;

						D3D12_TEXTURE_COPY_LOCATION dst = {}; // dst = 目标，默认堆gpu资源。
						dst.pResource = m_pTexture.Get();
						dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
						dst.SubresourceIndex = index;

						uint32_t dstX = 0;
						uint32_t dstY = rowSt;
						uint32_t dstZ = 0;

						D3D12_BOX box = {};
						box.left = 0;
						box.right = src.PlacedFootprint.Footprint.Width;
						box.top = 0;
						box.bottom = rowEd - rowSt;
						box.front = 0;
						box.back = 1;
						taskContext.pOwner->pCmdList->CopyTextureRegion(&dst, dstX, dstY, dstZ, &src, &box);

						NXUploadSystem->FinishTask(taskContext, [this]() {
							// 任务完成后的回调
							ProcessLoadingTexChunks();
							});
					}
					else
					{
						// chunk里有多个layout，累积处理
						for (int index = texChunk.layoutIndexStart; index < texChunk.layoutIndexStart + texChunk.layoutIndexSize; index++)
						{
							NXConvert::GetMipSliceFromLayoutIndex(index, texDesc.MipLevels, texDesc.DepthOrArraySize, mip, slice);

							const Image* pImg = pImage->GetImage(mip, slice, 0);

							const BYTE* pSrcData = pImg->pixels;
							BYTE* pMappedRingBufferData = taskContext.pResourceData;
							uint64_t bytesOffset = taskContext.pResourceOffset + layouts[index].Offset - layouts[texChunk.layoutIndexStart].Offset;
							BYTE* pDstData = pMappedRingBufferData + bytesOffset;

							for (uint32_t y = 0; y < numRow[index]; y++)
							{
								memcpy(pDstData + layouts[index].Footprint.RowPitch * y, pSrcData + pImg->rowPitch * y, numRowSizeInBytes[index]);
							}

							// NXUploadSystem 从RingBuffer同步到实际GPU资源
							D3D12_TEXTURE_COPY_LOCATION src = {}; // src = 上传堆中的内存段
							src.pResource = taskContext.pResource;
							src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
							src.PlacedFootprint = layouts[index];
							src.PlacedFootprint.Offset = bytesOffset;

							D3D12_TEXTURE_COPY_LOCATION dst = {}; // dst = 目标，默认堆gpu资源。
							dst.pResource = m_pTexture.Get();
							dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
							dst.SubresourceIndex = index;

							uint32_t dstX = 0;
							uint32_t dstY = 0;
							uint32_t dstZ = 0;
							taskContext.pOwner->pCmdList->CopyTextureRegion(&dst, dstX, dstY, dstZ, &src, nullptr);
						}

						NXUploadSystem->FinishTask(taskContext, [this]() {
							ProcessLoadingTexChunks();
							});
					}
				}
				else
				{
					// 抛出异常
					printf("Error: [NXUploadSystem::BuildTask] failed when loading NXTexture2D: %s\n", filePath.string().c_str());
				}
			}
			});
		};
	NXTexLoader->AddTask(task);
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
	case TextureType_None: 
		return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	case TextureType_1D: 
		return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	case Tex2D: 
	case TexCube:
	case Tex2DArray:
		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	case TextureType_3D:
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

Ntr<NXTexture2D> NXTexture2D::Create(const std::string& debugName, const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags)
{
	m_texFilePath = filePath;
	m_name = debugName;
	Deserialize();

	CreatePathTextureInternal(m_texFilePath, flags);
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
	m_width = texSize;
	m_height = texSize;
	m_arraySize = 1;
	m_mipLevels = 1;
	m_texFormat = fmt;

	CreateInternal(pImage, flags);

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
	m_width = texSize;
	m_height = texSize;
	m_arraySize = 1;
	m_mipLevels = 1;
	m_texFormat = fmt;

	CreateInternal(pImage, flags);

	pImage.reset();
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateHeightRaw(const std::string& debugName, const std::filesystem::path& rawPath, D3D12_RESOURCE_FLAGS flags)
{
	m_texFilePath = rawPath;
	Deserialize();

	int width = m_serializationData.m_rawWidth;
	int height = m_serializationData.m_rawHeight;

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
	uint16_t* p = reinterpret_cast<uint16_t*>(image.pixels);
	memcpy(p, rawData.data(), width * height * bytePerPixel);

	m_name = debugName;
	m_width = width;
	m_height = height;
	m_arraySize = 1;
	m_mipLevels = 1;
	m_texFormat = fmt;

	CreateInternal(pImage, flags);

	pImage.reset();
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

		ProcessLoadingBuffers();
		});
}

void NXTexture2D::SetRTV(uint32_t index)
{
	NXAllocator_RTV->Alloc([this, index](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pRTVs[index] = result;
		NXGlobalDX::GetDevice()->CreateRenderTargetView(m_pTexture.Get(), nullptr, m_pRTVs[index]);
		ProcessLoadingBuffers();
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

		ProcessLoadingBuffers();
		});
}

void NXTexture2D::SetUAV(uint32_t index)
{
	NXAllocator_SRV->Alloc([this, index](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pUAVs[index] = result;
		NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pTexture.Get(), nullptr, nullptr, m_pUAVs[index]);
		ProcessLoadingBuffers();
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

	CreatePathTextureInternal(m_texFilePath, flags);

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

		ProcessLoadingBuffers();
		});
}

D3D12_CPU_DESCRIPTOR_HANDLE NXTextureCube::GetSRVPreview(uint32_t index)
{
	WaitLoading2DPreviewFinish();
	return { m_pSRVPreviews[index] };
}

void NXTextureCube::SetSRVPreviews()
{
	m_loading2DPreviews = 6;
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

		ProcessLoadingBuffers();
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

		ProcessLoadingBuffers();
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

		ProcessLoadingBuffers();
		});
}

void NXTexture2DArray::Create(const std::string& debugName, const std::wstring& filePath, D3D12_RESOURCE_FLAGS flags)
{
	m_texFilePath = filePath.c_str();
	m_name = debugName;
	Deserialize();

	CreatePathTextureInternal(m_texFilePath, flags);
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

		ProcessLoadingBuffers();
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

		ProcessLoadingBuffers();
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

		ProcessLoadingBuffers();
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

		ProcessLoadingBuffers();
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
