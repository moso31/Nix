#pragma once
// ===== RenderGraphPassData.h =====
// 将所有 RenderGraph Pass 使用的 PassData 结构体 提取到这里
// 避免在 GenerateRenderGraph() 函数内部定义局部类型 + 模板实例化，从而改善 IntelliSense 性能

#include "NXRGUtil.h"
#include <vector>

// =====================================================
// Terrain Streaming Passes
// =====================================================

struct Sector2VirtImgClearPassData
{
	NXRGHandle Sector2VirtImg;
};

struct Sector2VirtImgPassData
{
	NXRGHandle Sector2VirtImg;
};

struct TerrainSector2NodeClearPassData
{
	NXRGHandle Sector2NodeTex;
};

struct TerrainAtlasBakerPassData
{
	std::vector<NXRGHandle> pIn;
	NXRGHandle pOutAtlas;
};

struct TerrainSector2NodeTintPassData
{
	NXRGHandle Sector2NodeTex;
};

// =====================================================
// Terrain Culling Passes
// =====================================================

struct TerrainNodesCullingPassData
{
	NXRGHandle sector2NodeTex;
	NXRGHandle pIn;
	NXRGHandle pOut;
	NXRGHandle pFinal;
	NXRGHandle pIndiArgs;
};

struct TerrainPatcherPassData
{
	NXRGHandle pFinal;
	NXRGHandle pPatcher;
	NXRGHandle pIndirectArgs;
	NXRGHandle pDrawIndexArgs;
};

// =====================================================
// Virtual Texture Passes
// =====================================================

struct PhysicalPageBakerPassData
{
	NXRGHandle Sector2NodeIDTex;
	NXRGHandle SplatMapAtlas;
	NXRGHandle AlbedoMapArray;
	NXRGHandle NormalMapArray;
	NXRGHandle PhysicalPageAlbedo;
	NXRGHandle PhysicalPageNormal;
};

struct IndirectTextureClearPassData
{
	NXRGHandle IndirectTexture;
};

struct UpdateIndirectTexturePassData
{
	NXRGHandle IndirectTexture;
};

struct PageIDTextureClearPassData
{
	NXRGHandle VTPageIDTexture;
};

struct RemoveIndirectTextureSectorPassData
{
	NXRGHandle IndirectTexture;
};

struct MigrateIndirectTextureSectorPassData
{
	NXRGHandle IndirectTexture;
};

// =====================================================
// GBuffer Passes
// =====================================================

struct GBufferPassData
{
	NXRGHandle depth;
	NXRGHandle rt0;
	NXRGHandle rt1;
	NXRGHandle rt2;
	NXRGHandle rt3;
	NXRGHandle VTPageIDTexture;
	NXRGHandle VTSector2VirtImg;
	NXRGHandle VTIndirectTexture;
	NXRGHandle VTPhysicalPageAlbedo;
	NXRGHandle VTPhysicalPageNormal;
};

struct VTReadbackPassData
{
	NXRGHandle vtReadback;
};

// =====================================================
// Shadow Passes
// =====================================================

struct ShadowMapPassData
{
	NXRGHandle csmDepth;
};

struct ShadowTestPassData
{
	NXRGHandle gbufferDepth;
	NXRGHandle csmDepth;
	NXRGHandle shadowTest;
};

// =====================================================
// Lighting Passes
// =====================================================

struct DeferredLightingPassData
{
	NXRGHandle gbuffer0;
	NXRGHandle gbuffer1;
	NXRGHandle gbuffer2;
	NXRGHandle gbuffer3;
	NXRGHandle gbufferDepth;
	NXRGHandle shadowTest;
	NXRGHandle cubeMap;
	NXRGHandle preFilter;
	NXRGHandle brdfLut;
	NXRGHandle lighting;
	NXRGHandle lightingSpec;
	NXRGHandle lightingCopy;
};

struct SubsurfacePassData
{
	NXRGHandle lighting;
	NXRGHandle lightingSpec;
	NXRGHandle gbuffer1;
	NXRGHandle noise64;
	NXRGHandle buf;
	NXRGHandle depth;
};

struct SkyLightingPassData
{
	NXRGHandle cubeMap;
	NXRGHandle buf;
	NXRGHandle depth;
};

// =====================================================
// Post Processing Passes
// =====================================================

struct PostProcessingPassData
{
	NXRGHandle skyBuf;
	NXRGHandle out;
};

struct DebugLayerPassData
{
	NXRGHandle postProcessOut;
	NXRGHandle csmDepth;
	NXRGHandle out;
};

struct GizmosPassData
{
	NXRGHandle out;
};

struct FinalQuadPassData
{
	NXRGHandle gizmosOut;
	NXRGHandle finalOut;
};

// =====================================================
// GPU Terrain Patcher Data (for inter-pass communication)
// =====================================================

struct GPUTerrainPatcherPassData
{
	NXRGHandle pMinMaxZMap;
	NXRGHandle pFinal;
	NXRGHandle pIndiArgs;
	NXRGHandle pPatcher;
	NXRGHandle pDrawIndexArgs;
};
