#include "NXTextureMaker.h"
#include "NXConverter.h"
#include <DirectXTex.h>
#include "NXTerrainCommon.h"

using namespace DirectX;

void NXTextureMaker::ReadTerrainRawR16(const std::filesystem::path& path, std::vector<uint16_t>& out)
{
    const uint32_t kBytesPerPixel = sizeof(uint16_t);

    if (!NXConvert::IsRawFileExtension(path.extension().string()))
        throw std::runtime_error("高度图扩展名不是 .raw");

    const uint64_t expectedBytes = uint64_t(kBaseSize) * uint64_t(kBaseSize) * kBytesPerPixel;

    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("打开 RAW 失败: " + path.string());

    out.resize(size_t(kBaseSize) * size_t(kBaseSize));
    file.read(reinterpret_cast<char*>(out.data()), std::streamsize(expectedBytes));
    if (file.gcount() != std::streamsize(expectedBytes))
        throw std::runtime_error("RAW 尺寸不为 2049x2049 R16: " + path.string());
}

void NXTextureMaker::ReadTerrainDDSR8Unorm(const std::filesystem::path& path, std::vector<uint8_t>& out)
{
    TexMetadata meta;
    ScratchImage img;
    HRESULT hr = LoadFromDDSFile(path.wstring().c_str(), DDS_FLAGS_NONE, &meta, img);
    if (FAILED(hr)) 
    {
        throw std::runtime_error("LoadFromDDSFile 失败: " + path.string());
    }

    // 2) 检查格式必须是 R8_UNORM，否则直接报错
    DXGI_FORMAT fmt = meta.format;
    if (fmt != DXGI_FORMAT_R8_UNORM)
    {
        throw std::runtime_error("DDS 格式必须是 R8_UNORM: " + path.string());
    }

    const Image* src = img.GetImage(0, 0, 0);

    if (!src) 
        throw std::runtime_error("获取图像数据失败: " + path.string());

    // 3) 基本校验：只读 2D（允许有 mip/array，但此处取 slice0/mip0）
    if (src->format != DXGI_FORMAT_R8_UNORM) 
        throw std::runtime_error("DDS 非 8bit 单通道格式: " + path.string());

    const size_t width = src->width;
    const size_t height = src->height;
    const size_t rowPitch = src->rowPitch;

    out.resize(width * height);

    // 4) 逐行拷贝，忽略对齐字节
    const uint8_t* p = src->pixels;
    for (size_t y = 0; y < height; ++y) 
    {
        const uint8_t* row = p + y * rowPitch;
        std::memcpy(&out[y * width], row, width);
    }
}

void NXTextureMaker::EnsureDir(const std::filesystem::path& dir)
{
    std::error_code ec;
    if (!std::filesystem::create_directories(dir, ec))
    {
        // 已存在或创建成功都可以；只有在真正失败时抛
        if (!std::filesystem::exists(dir))
            throw std::runtime_error("创建目录失败: " + dir.string());
    }
}

void NXTextureMaker::SaveTerrainTileHeightMap(const std::filesystem::path& outPath, const uint16_t* src, uint32_t srcW, uint32_t srcH, uint32_t startX, uint32_t startY, uint32_t tileSize)
{
    const uint32_t kMinTileSize = g_terrainConfig.SectorSize + 1; // 65

    // 输出大小=kMinTileSize的Tile，如果输入tileSize超过这个大小，就做降采样
    ScratchImage img;
    HRESULT hr = img.Initialize2D(kHeightMapFormat, kMinTileSize, kMinTileSize, /*arraySize*/1, /*mipLevels*/1);
    if (FAILED(hr)) throw std::runtime_error("ScratchImage::Initialize2D 失败");

    const Image* dst = img.GetImage(0, 0, 0);
    uint8_t* dstBase = dst->pixels;
    const size_t dstRowPitch = dst->rowPitch;

    // 专门针对2整数幂的点降采样
    const uint32_t step = (tileSize - 1u) / (kMinTileSize - 1u); 
    for (uint32_t y = 0; y < kMinTileSize; ++y)
    {
        const uint32_t sy = startY + y * step;
        const uint16_t* srcRow = src + size_t(sy) * srcW;

        uint16_t* dstRow = reinterpret_cast<uint16_t*>(dstBase + size_t(y) * dstRowPitch);
        for (uint32_t x = 0; x < kMinTileSize; ++x)
        {
            const uint32_t sx = startX + x * step;
            dstRow[x] = srcRow[sx];
        }
    }

    hr = SaveToDDSFile(img.GetImages(), img.GetImageCount(), img.GetMetadata(), DDS_FLAGS_NONE, outPath.wstring().c_str());
    if (FAILED(hr))
        throw std::runtime_error("保存 DDS 失败: " + outPath.string());
}

void NXTextureMaker::SaveTerrainTileSplatMap(const std::filesystem::path& outPath, const uint8_t* src, uint32_t srcW, uint32_t srcH, uint32_t startX, uint32_t startY, uint32_t tileSize)
{
    const uint32_t kMinTileSize = g_terrainConfig.SectorSize + 1; // 65

    // 输出大小=kMinTileSize的Tile，如果输入tileSize超过这个大小，就做降采样
    ScratchImage img;
    HRESULT hr = img.Initialize2D(DXGI_FORMAT_R8_UNORM, kMinTileSize, kMinTileSize, /*arraySize*/1, /*mipLevels*/1);
    if (FAILED(hr)) throw std::runtime_error("ScratchImage::Initialize2D 失败");

    const Image* dst = img.GetImage(0, 0, 0);
    uint8_t* dstBase = dst->pixels;
    const size_t dstRowPitch = dst->rowPitch;

    // 专门针对2整数幂的点降采样
    const uint32_t step = (tileSize - 1u) / (kMinTileSize - 1u); 
    for (uint32_t y = 0; y < kMinTileSize; ++y)
    {
        const uint32_t sy = startY + y * step;
        const uint8_t* srcRow = src + size_t(sy) * srcW;

        uint8_t* dstRow = dstBase + size_t(y) * dstRowPitch;
        for (uint32_t x = 0; x < kMinTileSize; ++x)
        {
            const uint32_t sx = startX + x * step;
            dstRow[x] = srcRow[sx];
        }
    }

    hr = SaveToDDSFile(img.GetImages(), img.GetImageCount(), img.GetMetadata(), DDS_FLAGS_NONE, outPath.wstring().c_str());
    if (FAILED(hr))
        throw std::runtime_error("保存 SplatMap DDS 失败: " + outPath.string());
}

void NXTextureMaker::GenerateTerrainHeightMap2DArray(const TerrainTexLODBakeConfig& bakeConfig, uint32_t nodeCountX, uint32_t nodeCountY, uint32_t width, uint32_t height, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount)
{
    auto& rawPaths = bakeConfig.bakeTerrains;

    uint32_t arraySize = nodeCountX * nodeCountY;
    arraySize = std::min<uint32_t>(arraySize, static_cast<uint32_t>(rawPaths.size()));
    if (arraySize == 0)
        throw std::runtime_error("arraySize == 0");

    constexpr DXGI_FORMAT kFormat = DXGI_FORMAT_R16_UNORM;
    const uint32_t kBytesPerPixel = sizeof(uint16_t);

    // 创建 2D array 纹理
    std::unique_ptr<ScratchImage> texArray = std::make_unique<ScratchImage>();
    HRESULT hr = texArray->Initialize2D(kFormat, width, height, rawPaths.size(), 1);
    if (FAILED(hr))
        throw std::runtime_error("DirectXTex::InitializeArray 失败");

    for (uint32_t i = 0; i < arraySize; ++i)
    {
        int slice = rawPaths[i].nodeId.y * nodeCountX + rawPaths[i].nodeId.x;
        const auto& path = rawPaths[i].pathHeightMap;
        std::vector<uint16_t> rawData(width * height);

        bool rawValid = true;
        if (!NXConvert::IsRawFileExtension(path.extension().string()))
            rawValid = false;

        std::ifstream file(path, std::ios::binary);
        if (!file) 
            rawValid = false;

        file.read(reinterpret_cast<char*>(rawData.data()), width * height * kBytesPerPixel);
        if (file.gcount() != static_cast<std::streamsize>(width * height * kBytesPerPixel)) 
            rawValid = false;

        if (!rawValid) // 如果不是 raw 文件，或者raw的格式不对，对应slice填充全黑纹理
        {
            std::fill(rawData.begin(), rawData.end(), uint16_t(0)); 
        }

        // 写入对应 i
        const Image* dst = texArray->GetImage(0, slice, 0);
        std::memcpy(dst->pixels, rawData.data(), width* height* kBytesPerPixel);

        // 如有必要，通知外部计数器-1
        if (onProgressCount) onProgressCount();
    }

    // 2) 压缩到 BC4（并行）
    //std::unique_ptr<ScratchImage> bc4 = std::make_unique<ScratchImage>();
    //{
    //    const TexMetadata& meta = texArray->GetMetadata();
    //    TEX_COMPRESS_FLAGS cflags = TEX_COMPRESS_DEFAULT | TEX_COMPRESS_PARALLEL;
    //    HRESULT hr = Compress(texArray->GetImages(), texArray->GetImageCount(), meta, DXGI_FORMAT_BC4_UNORM, cflags, 0.5f, *bc4);
    //    if (FAILED(hr)) 
    //        throw std::runtime_error("Compress(BC4) 失败");

    //    texArray.swap(bc4);
    //}

    // 保存到 DDS
    hr = SaveToDDSFile(texArray->GetImages(), texArray->GetImageCount(), texArray->GetMetadata(), DDS_FLAGS_NONE, outDDSPath.wstring().c_str());
    if (FAILED(hr))
        throw std::runtime_error("保存 DDS 失败: " + outDDSPath.string());
}

void NXTextureMaker::GenerateTerrainMinMaxZMap2DArray(const TerrainTexLODBakeConfig& bakeConfig, uint32_t nodeCountX, uint32_t nodeCountY, uint32_t width, uint32_t height, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount)
{
    auto& inPaths = bakeConfig.bakeTerrains;

    uint32_t arraySize = nodeCountX * nodeCountY; 
    arraySize = std::min<uint32_t>(arraySize, static_cast<uint32_t>(inPaths.size()));
    if (arraySize == 0)
        throw std::runtime_error("arraySize == 0");

    const int  step = 8;
    uint32_t   mip0Width = width / step; // 2049 会被自动取整抹掉余数
    uint32_t   mip0Height = height / step;
    const int  mipLevels = 6;

    auto pImage = std::make_shared<ScratchImage>();
    HRESULT hr = pImage->Initialize2D(DXGI_FORMAT_R32G32_FLOAT, mip0Width, mip0Height, arraySize, mipLevels);
    if (FAILED(hr))
        throw std::runtime_error("DirectXTex::Initialize2D 失败");

    const uint32_t kBytesPerPixel = sizeof(uint16_t);

    for (uint32_t i = 0; i < arraySize; ++i)
    {
        int slice = inPaths[i].nodeId.y * nodeCountX + inPaths[i].nodeId.x;
        const auto& path = inPaths[i].pathHeightMap;
        std::vector<uint16_t> rawData(width * height, 0);

        if (NXConvert::IsRawFileExtension(path.extension().string()))
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
                throw std::runtime_error("无法打开文件: " + path.string());

            file.read(reinterpret_cast<char*>(rawData.data()),
                static_cast<std::streamsize>(rawData.size() * kBytesPerPixel));
            if (!file)
                throw std::runtime_error("读取数据失败: " + path.string());
        }

        std::vector<MinMaxZMap> dataZMip0(mip0Width * mip0Height);

        const static int s_maxHeight = 2048;
        const static int s_minHeight = 0;

        for (int y = 0; y + step < static_cast<int>(height); y += step)
        {
            for (int x = 0; x + step < static_cast<int>(width); x += step)
            {
                float minZ = std::numeric_limits<float>::max();
                float maxZ = std::numeric_limits<float>::lowest();

                for (int j = 0; j <= step; ++j)
                {
                    for (int i = 0; i <= step; ++i)
                    {
                        int yy = std::min(y + j, static_cast<int>(height) - 1);
                        int xx = std::min(x + i, static_cast<int>(width) - 1);

                        uint16_t value = rawData[yy * width + xx];
                        float normalizedV = static_cast<float>(value) / 65535.0f;   // [0,1]

                        minZ = std::min(minZ, normalizedV);
                        maxZ = std::max(maxZ, normalizedV);
                    }
                }

                // remap to [s_minHeight, s_maxHeight]
                minZ = minZ * (s_maxHeight - s_minHeight) + s_minHeight; 
                maxZ = maxZ * (s_maxHeight - s_minHeight) + s_minHeight;

                dataZMip0[(y / step) * mip0Width + (x / step)].minVal = minZ;
                dataZMip0[(y / step) * mip0Width + (x / step)].maxVal = maxZ;
            }
        }

        const int  mipStep = 2;
        uint32_t   prevW = mip0Width;
        uint32_t   prevH = mip0Height;
        std::vector<std::vector<MinMaxZMap>> dataZMip1To5(5);

        for (int mip = 0; mip < 5; ++mip)
        {
            uint32_t currW = prevW / mipStep;
            uint32_t currH = prevH / mipStep;
            dataZMip1To5[mip].resize(currW * currH);

            for (uint32_t y = 0; y < prevH; y += mipStep)
            {
                for (uint32_t x = 0; x < prevW; x += mipStep)
                {
                    float minZ = std::numeric_limits<float>::max();
                    float maxZ = std::numeric_limits<float>::lowest();

                    for (uint32_t j = 0; j < mipStep; ++j)
                    {
                        for (uint32_t i = 0; i < mipStep; ++i)
                        {
                            uint32_t index = (y + j) * prevW + (x + i);
                            const MinMaxZMap& src = (mip == 0)
                                ? dataZMip0[index]
                                : dataZMip1To5[mip - 1][index];

                            minZ = std::min(minZ, src.minVal);
                            maxZ = std::max(maxZ, src.maxVal);
                        }
                    }

                    dataZMip1To5[mip][(y / mipStep) * currW + (x / mipStep)].minVal = minZ;
                    dataZMip1To5[mip][(y / mipStep) * currW + (x / mipStep)].maxVal = maxZ;
                }
            }

            prevW = currW;
            prevH = currH;
        }

        auto copyLevel = [&](uint32_t mip, const std::vector<MinMaxZMap>& src)
            {
                const Image* dst = pImage->GetImage(mip, slice, 0);
                MinMaxZMap* pDst = reinterpret_cast<MinMaxZMap*>(dst->pixels);
                std::memcpy(pDst, src.data(), src.size() * sizeof(MinMaxZMap));
            };

        copyLevel(0, dataZMip0);
        for (int mip = 0; mip < 5; ++mip)
            copyLevel(mip + 1, dataZMip1To5[mip]);

        // 如有必要，通知外部计数器-1
        if (onProgressCount) onProgressCount();
    }

    hr = SaveToDDSFile(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), DDS_FLAGS_NONE, outDDSPath.wstring().c_str());
    if (FAILED(hr))
        throw std::runtime_error("保存 DDS 失败: " + outDDSPath.string());
}

void NXTextureMaker::GenerateTerrainNormal2DArray(const TerrainTexLODBakeConfig& bakeConfig, uint32_t nodeCountX, uint32_t nodeCountY, uint32_t width, uint32_t height, const Vector2& zRange, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount)
{
    auto& rawPaths = bakeConfig.bakeTerrains;

    uint32_t arraySize = nodeCountX * nodeCountY;
    arraySize = std::min<uint32_t>(arraySize, static_cast<uint32_t>(rawPaths.size()));
    if (arraySize == 0)
        throw std::runtime_error("arraySize == 0");

    constexpr DXGI_FORMAT kFormat = DXGI_FORMAT_R10G10B10A2_UNORM;  // 输出格式
    constexpr uint32_t kBytesPerPixel = sizeof(uint16_t);

    std::unique_ptr<ScratchImage> texArray = std::make_unique<ScratchImage>();
    HRESULT hr = texArray->Initialize2D(kFormat, width, height, arraySize, 1);
    if (FAILED(hr))
        throw std::runtime_error("DirectXTex::Initialize2D 失败");

    std::vector<uint16_t> rawData(width * height);
    std::vector<Vector3> normalBuf(width * height);

    for (uint32_t n = 0; n < arraySize; ++n)
    {
        int slice = rawPaths[n].nodeId.y * nodeCountX + rawPaths[n].nodeId.x;
        const auto& path = rawPaths[n].pathHeightMap;

        bool rawOK = NXConvert::IsRawFileExtension(path.extension().string());
        if (rawOK)
        {
            std::ifstream ifs(path, std::ios::binary);
            if (!ifs) rawOK = false;
            if (rawOK)
            {
                ifs.read(reinterpret_cast<char*>(rawData.data()), width * height * kBytesPerPixel);
                if (ifs.gcount() != static_cast<std::streamsize>(width * height * kBytesPerPixel))
                    rawOK = false;
            }
        }

        if (!rawOK)
        {
            std::fill(rawData.begin(), rawData.end(), 0);
        }

        auto heightWorld = [&](uint16_t h16) -> float
            {
                float h = static_cast<float>(h16) / 65535.0f;   // [0,1]
                return h * (zRange.y - zRange.x) + zRange.x;    // [zMin,zMax]
            };

        /* ----------- 3. 遍历像素求法线 ----------- */
        for (uint32_t j = 0; j < height; ++j)
        {
            uint32_t jU = (j == 0) ? j : j - 1;
            uint32_t jD = (j == height - 1) ? j : j + 1;

            for (uint32_t i = 0; i < width; ++i)
            {
                uint32_t iL = (i == 0) ? i : i - 1;
                uint32_t iR = (i == width - 1) ? i : i + 1;

                float hL = heightWorld(rawData[j * width + iL]);
                float hR = heightWorld(rawData[j * width + iR]);
                float hU = heightWorld(rawData[jU * width + i]);
                float hD = heightWorld(rawData[jD * width + i]);

                // L/R/U/D 在世界坐标中的位置
                Vector3 posL((float)iL, hL, (float)j);
                Vector3 posR((float)iR, hR, (float)j);
                Vector3 posU((float)i, hU, (float)jU);
                Vector3 posD((float)i, hD, (float)jD);

                Vector3 vecLR = posR - posL;
                Vector3 vecUD = posD - posU;
                Vector3 nrm = vecUD.Cross(vecLR);
                nrm.Normalize();

                normalBuf[j * width + i] = nrm;
            }
        }

        const Image* dstImg = texArray->GetImage(0, slice, 0);
        uint8_t* dstPtr = dstImg->pixels;

        for (uint32_t y = 0; y < height; ++y)
        {
            uint32_t* row = reinterpret_cast<uint32_t*>(dstPtr + y * dstImg->rowPitch);
            for (uint32_t x = 0; x < width; ++x)
            {
                const Vector3& n = normalBuf[y * width + x];
                uint32_t r = static_cast<uint32_t>((n.x * 0.5f + 0.5f) * 1023.0f);
                uint32_t g = static_cast<uint32_t>((n.y * 0.5f + 0.5f) * 1023.0f);
                uint32_t b = static_cast<uint32_t>((n.z * 0.5f + 0.5f) * 1023.0f);
                uint32_t a = 3;   // 2-bit alpha 置满

                row[x] = (a << 30) | (b << 20) | (g << 10) | (r << 0);
            }
        }

        if (onProgressCount) onProgressCount();
    }

    if (!outDDSPath.empty())
    {
        hr = SaveToDDSFile(texArray->GetImages(), texArray->GetImageCount(),
            texArray->GetMetadata(), DDS_FLAGS_NONE,
            outDDSPath.wstring().c_str());
        if (FAILED(hr))
            throw std::runtime_error("保存 DDS 失败: " + outDDSPath.string());
    }
}

void NXTextureMaker::GenerateTerrainStreamingLODMaps(const TerrainTexLODBakeConfig& bakeConfig)
{
    if (bakeConfig.bGenerateHeightMap)
    {
        GenerateTerrainStreamingLODMaps_HeightMap(bakeConfig);
    }
    if (bakeConfig.bGenerateSplatMap)
    {
        GenerateTerrainStreamingLODMaps_SplatMap(bakeConfig);
    }
}

void NXTextureMaker::GenerateTerrainStreamingLODMaps_HeightMap(const TerrainTexLODBakeConfig& bakeConfig)
{
    const uint32_t kMinTileSize = g_terrainConfig.SectorSize + 1; // 65
    auto& rawPaths = bakeConfig.bakeTerrains;

    for (const auto& item : rawPaths)
    {
        const auto& rawPath = item.pathHeightMap;

        // 1) 读取原始 R16 RAW
        std::vector<uint16_t> base;
        ReadTerrainRawR16(rawPath, base);

        // 2) 输出目录：<tile_dir>\sub\hmap\ 
        const std::filesystem::path tileDir = rawPath.parent_path();
        const std::filesystem::path outDir = tileDir / "sub" / "hmap";
        EnsureDir(outDir);

        // 3) 遍历 LOD 层级（0..5）
        for (uint32_t L = 0; ; ++L)
        {
            const uint32_t nTiles = 1u << L;
            const uint32_t tileSize = (kBaseSize - 1u) / nTiles + 1u; // 2049 -> 1025 -> 513 -> ...
            if (tileSize < kMinTileSize) break;                       // 到 65 为止（LOD5）

            const uint32_t stride = tileSize - 1u; // 子块起点步长（含重叠边）

            for (uint32_t ty = 0; ty < nTiles; ++ty)
            {
                const uint32_t startY = ty * stride;

                for (uint32_t tx = 0; tx < nTiles; ++tx)
                {
                    const uint32_t startX = tx * stride;

                    // 文件名：<size>_<row>_<col>.dds 例如：1025_1_0.dds
                    std::wstring fname = std::to_wstring(tileSize) + L"_" +
                        std::to_wstring(ty) + L"_" +
                        std::to_wstring(tx) + L".dds";
                    const auto outPath = outDir / fname;

                    // 检查文件是否存在，如果不强制生成且文件已存在则跳过
                    if (!bakeConfig.bForceGenerate && std::filesystem::exists(outPath))
                    {
                        printf("跳过已存在的 HeightMap LOD%d tile (%d,%d) : %s\n", L, tx, ty, outPath.string().c_str());
                        continue;
                    }

                    printf("生成 HeightMap LOD%d tile (%d,%d) : %s\n", L, tx, ty, outPath.string().c_str());
                    SaveTerrainTileHeightMap(outPath, base.data(), kBaseSize, kBaseSize, startX, startY, tileSize);
                }
            }
        }
    }
}

void NXTextureMaker::GenerateTerrainStreamingLODMaps_SplatMap(const TerrainTexLODBakeConfig& bakeConfig)
{
    const uint32_t kMinTileSize = g_terrainConfig.SectorSize + 1; // 65
    auto& rawPaths = bakeConfig.bakeTerrains;

    for (const auto& item : rawPaths)
    {
        const auto& rawPath = item.pathSplatMap;

        // 1) 读取原始 R8 DDS
        std::vector<uint8_t> base;
        ReadTerrainDDSR8Unorm(rawPath, base);

        // 2) 输出目录：<tile_dir>\sub\splat\ 
        const std::filesystem::path tileDir = rawPath.parent_path();
        const std::filesystem::path outDir = tileDir / "sub" / "splat";
        EnsureDir(outDir);

        // 3) 遍历 LOD 层级（0..5）
        for (uint32_t L = 0; ; ++L)
        {
            const uint32_t nTiles = 1u << L;
            const uint32_t tileSize = (kBaseSize - 1u) / nTiles + 1u; // 2049 -> 1025 -> 513 -> ...
            if (tileSize < kMinTileSize) break;                       // 到 65 为止（LOD5）

            const uint32_t stride = tileSize - 1u; // 子块起点步长（含重叠边）

            for (uint32_t ty = 0; ty < nTiles; ++ty)
            {
                const uint32_t startY = ty * stride;

                for (uint32_t tx = 0; tx < nTiles; ++tx)
                {
                    const uint32_t startX = tx * stride;

                    // 文件名：<size>_<row>_<col>.dds 例如：1025_1_0.dds
                    std::wstring fname = std::to_wstring(tileSize) + L"_" +
                        std::to_wstring(ty) + L"_" +
                        std::to_wstring(tx) + L".dds";
                    const auto outPath = outDir / fname;

                    // 检查文件是否存在，如果不强制生成且文件已存在则跳过
                    if (!bakeConfig.bForceGenerate && std::filesystem::exists(outPath))
                    {
                        printf("跳过已存在的 SplatMap LOD%d tile (%d,%d) : %s\n", L, tx, ty, outPath.string().c_str());
                        continue;
                    }

                    printf("生成 SplatMap LOD%d tile (%d,%d) : %s\n", L, tx, ty, outPath.string().c_str());
                    SaveTerrainTileSplatMap(outPath, base.data(), kBaseSize, kBaseSize, startX, startY, tileSize);
                }
            }
        }
    }
}
