#include "NXTextureMaker.h"
#include "NXConverter.h"
#include <DirectXTex.h>
#include "NXTerrainCommon.h"

using namespace DirectX;

void NXTextureMaker::GenerateTerrainHeightMap2DArray(const std::vector<std::filesystem::path>& rawPaths, uint32_t width, uint32_t height, uint32_t arraySize, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount)
{
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

    // 逐 slice 填充数据
    for (uint32_t slice = 0; slice < arraySize; ++slice)
    {
        const auto& path = rawPaths[slice];
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

        // 写入对应 slice
        const Image* dst = texArray->GetImage(0, slice, 0);
        std::memcpy(dst->pixels, rawData.data(), width* height* kBytesPerPixel);

        // 如有必要，通知外部计数器-1
        if (onProgressCount) onProgressCount();
    }

    // 保存到 DDS
    hr = SaveToDDSFile(texArray->GetImages(), texArray->GetImageCount(), texArray->GetMetadata(), DDS_FLAGS_NONE, outDDSPath.wstring().c_str());
    if (FAILED(hr))
        throw std::runtime_error("保存 DDS 失败: " + outDDSPath.string());
}

void NXTextureMaker::GenerateTerrainMinMaxZMap2DArray(const std::vector<std::filesystem::path>& inPaths, uint32_t width, uint32_t height, uint32_t arraySize, const std::filesystem::path& outDDSPath, std::function<void()> onProgressCount)
{
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

    for (uint32_t slice = 0; slice < arraySize; ++slice)
    {
        const auto& path = inPaths[slice];
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