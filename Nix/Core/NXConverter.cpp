#include "NXConverter.h"
#include <cwctype>

namespace NXConvert
{

std::string ws2s(const std::wstring& ws)
{
	std::string s;
	s.resize(ws.length());
	if (ws.empty()) return s;

	size_t size;
	wcstombs_s(&size, &s[0], s.size() + 1, ws.c_str(), ws.size());
	return s;
}

std::wstring s2ws(const std::string& s)
{
	return std::wstring(s.begin(), s.end());
}

std::string s2lower(const std::string& s)
{
	std::string result;
	std::transform(s.begin(), s.end(), std::back_inserter(result), ::tolower);
	return result;
}

std::wstring s2lower(const std::wstring& s)
{
	std::wstring result;
	std::transform(s.begin(), s.end(), std::back_inserter(result), ::towlower);
	return result;
}

std::string Trim(std::string& str)
{
	// ȥ�� str �е����пո��tab
	const auto begin = str.find_first_not_of(" \t");
	if (begin == std::string::npos) return "";
	const auto end = str.find_last_not_of(" \t");
	return str.substr(begin, end - begin + 1);
}

std::string GetPathOfImguiIni()
{
	char* buf = nullptr;
	size_t sz;
	_dupenv_s(&buf, &sz, "APPDATA");

	std::string s(buf); 
	free(buf);
	return s + "\\Nix\\imgui.ini";
}

std::string GetTitleOfFunctionData(const std::string& functionString)
{
	bool bInCommentLine = false;

	// while ���ж�ȡ�ı�
	std::string strFunc = functionString;
	while (true)
	{
		std::size_t line_end = strFunc.find_first_of("\n\r", 0);
		if (line_end == std::string::npos)
			break;

		std::string line = strFunc.substr(0, line_end);
		strFunc = strFunc.substr(line_end + 1);

		std::string lineNoComment;
		if (!bInCommentLine)
		{
			// ��ȡ�޵���ע�Ͳ��֣��� "//" ֮ǰ�����ݡ�
			lineNoComment = line.substr(0, line.find("//"));
			if (lineNoComment.empty()) continue;

			// �����Ƿ��ж���ע�Ϳ�ͷ "/*"��
			// ����У���һ���� lineNoComment �ָ��������
			auto nCommentMultiLineStartPos = lineNoComment.find("/*");
			if (nCommentMultiLineStartPos != std::string::npos)
			{
				lineNoComment = lineNoComment.substr(0, nCommentMultiLineStartPos);
				bInCommentLine = true;
			}
		}
		else
		{
			// ����ڶ���ע���У�������Ƿ��ж���ע�ͽ�β "*/"��
			// ����У��� bInCommentLine ����Ϊ false����ʾ����ע���ѽ�����
			auto commentPos = lineNoComment.find("*/");
			if (commentPos != std::string::npos)
			{
				lineNoComment = lineNoComment.substr(commentPos + 2);
				bInCommentLine = false;
			}
			else continue; // ����ڶ���ע���У���û�ж���ע�ͽ�β������Դ��С�
		}

		if (lineNoComment.empty()) continue;

		// 2023.7.29 Ĭ��ȥ��ע�ͺ�ĵ�һ�����������Ǻ�������
		return lineNoComment;
	}

	return "unknownFunction...";
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

void getline_safe(std::ifstream& ifs, std::string& s)
{
	std::getline(ifs, s);
	s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
	s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
}

bool IsMaterialDefaultPath(const std::string& s)
{
	return s == "?";
}

}
