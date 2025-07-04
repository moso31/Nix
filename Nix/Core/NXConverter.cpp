#include "NXConverter.h"
#include <cwctype>

namespace NXConvert
{

std::string GetPathOfImguiIni()
{
	char* buf = nullptr;
	size_t sz;
	_dupenv_s(&buf, &sz, "APPDATA");

	std::string s(buf); 
	free(buf);
	return s + "\\Nix\\imgui.ini";
}

bool IsUnormFormat(DXGI_FORMAT fmt) 
{
	switch (fmt) 
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_R1_UNORM:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		return true;

	default:
		return false;
	}
}

bool IsUncompressedFormat(DXGI_FORMAT fmt) 
{
	switch (fmt) 
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_R1_UNORM:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return true;

	default:
		return false;
	}
}

DXGI_FORMAT SafeDXGIFormat(DXGI_FORMAT fmt)
{
	switch (fmt) 
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return fmt;

	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_R1_UNORM:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;

	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC3_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
		
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	default:
		return fmt;
	}
}

DXGI_FORMAT DXGINoTypeless(DXGI_FORMAT fmt, bool isDepthStencil)
{
switch (fmt)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
		return DXGI_FORMAT_R32G32B32_FLOAT;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		return DXGI_FORMAT_R16G16B16A16_UNORM;

	case DXGI_FORMAT_R32G32_TYPELESS:
		return DXGI_FORMAT_R32G32_FLOAT;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		return DXGI_FORMAT_R10G10B10A2_UNORM;

	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		return DXGI_FORMAT_R8G8B8A8_UNORM;

	case DXGI_FORMAT_R16G16_TYPELESS:
		return DXGI_FORMAT_R16G16_UNORM;

	case DXGI_FORMAT_R32_TYPELESS:
		return isDepthStencil ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_R32_FLOAT;

	case DXGI_FORMAT_R8G8_TYPELESS:
		return DXGI_FORMAT_R8G8_UNORM;

	case DXGI_FORMAT_R16_TYPELESS:
		return isDepthStencil ? DXGI_FORMAT_D16_UNORM : DXGI_FORMAT_R16_FLOAT;

	case DXGI_FORMAT_R8_TYPELESS:
		return DXGI_FORMAT_R8_UNORM;

	case DXGI_FORMAT_BC1_TYPELESS:
		return DXGI_FORMAT_BC1_UNORM;

	case DXGI_FORMAT_BC2_TYPELESS:
		return DXGI_FORMAT_BC2_UNORM;

	case DXGI_FORMAT_BC3_TYPELESS:
		return DXGI_FORMAT_BC3_UNORM;

	case DXGI_FORMAT_BC4_TYPELESS:
		return DXGI_FORMAT_BC4_UNORM;

	case DXGI_FORMAT_BC5_TYPELESS:
		return DXGI_FORMAT_BC5_UNORM;

	case DXGI_FORMAT_BC6H_TYPELESS:
		return DXGI_FORMAT_BC6H_UF16;

	case DXGI_FORMAT_BC7_TYPELESS:
		return DXGI_FORMAT_BC7_UNORM;

	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;

	default:
		return fmt;
	}
}

DXGI_FORMAT TypelessToDSVFormat(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT ForceSRGB(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	case DXGI_FORMAT_BC1_UNORM:
		return DXGI_FORMAT_BC1_UNORM_SRGB;

	case DXGI_FORMAT_BC2_UNORM:
		return DXGI_FORMAT_BC2_UNORM_SRGB;

	case DXGI_FORMAT_BC3_UNORM:
		return DXGI_FORMAT_BC3_UNORM_SRGB;

	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

	case DXGI_FORMAT_BC7_UNORM:
		return DXGI_FORMAT_BC7_UNORM_SRGB;

	default:
		return fmt;
	}
}

DXGI_FORMAT ForceLinear(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM;

	case DXGI_FORMAT_BC1_UNORM_SRGB:
		return DXGI_FORMAT_BC1_UNORM;

	case DXGI_FORMAT_BC2_UNORM_SRGB:
		return DXGI_FORMAT_BC2_UNORM;

	case DXGI_FORMAT_BC3_UNORM_SRGB:
		return DXGI_FORMAT_BC3_UNORM;

	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM;

	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8X8_UNORM;

	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return DXGI_FORMAT_BC7_UNORM;

	default:
		return fmt;
	}
}

bool IsImageFileExtension(const std::string& strExtension)
{
	std::string s = s2lower(strExtension);
	return s == ".dds" || s == ".png" || s == ".jpg" || s == ".tga" || s == ".bmp" || s == ".hdr";
}

bool IsDDSFileExtension(const std::string& strExtension)
{
	std::string s = s2lower(strExtension);
	return s == ".dds";
}

bool IsRawFileExtension(const std::string& strExtension)
{
	std::string s = s2lower(strExtension);
	return s == ".raw";
}

bool IsMaterialFileExtension(const std::string& strExtension)
{
	std::string s = s2lower(strExtension);
	return s == ".nsl";
}

bool IsDiffuseProfileExtension(const std::string& strExt)
{
	std::string s = s2lower(strExt);
	return s == ".nssprof";
}

bool IsTerrainLayerExtension(const std::string& strExt)
{
	std::string s = s2lower(strExt);
	return s == ".ntl";
}

bool IsMaterialDefaultPath(const std::string& s)
{
	return s == "?";
}

void GetMipSliceFromLayoutIndex(int layoutIndex, int mipSize, int sliceSize, int& oMip, int& oSlice)
{
	oMip = layoutIndex % mipSize;
	oSlice = (layoutIndex / mipSize) % sliceSize;
}

bool GetMetadataFromFile(const std::filesystem::path& path, DirectX::TexMetadata& oMetaData)
{
	using namespace DirectX;

	HRESULT hr;
	std::string strExtension = NXConvert::s2lower(path.extension().string());
	if (strExtension == ".hdr")
		hr = GetMetadataFromHDRFile(path.wstring().c_str(), oMetaData);
	else if (strExtension == ".dds")
		hr = GetMetadataFromDDSFile(path.wstring().c_str(), DDS_FLAGS_NONE, oMetaData);
	else if (strExtension == ".tga")
		hr = GetMetadataFromTGAFile(path.wstring().c_str(), oMetaData);
	else
		hr = GetMetadataFromWICFile(path.wstring().c_str(), WIC_FLAGS_NONE, oMetaData);

	return SUCCEEDED(hr);
}

}
