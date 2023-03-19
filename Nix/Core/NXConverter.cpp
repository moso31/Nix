#include "NXConverter.h"
#include <cwctype>
#include <fstream>

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

std::string s2lower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), std::tolower);
	return s;
}

std::wstring s2lower(std::wstring s)
{
	std::transform(s.begin(), s.end(), s.begin(), std::towlower);
	return s;
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

bool IsImageFileExtension(const std::string& strExtension)
{
	std::string s = s2lower(strExtension);
	return s == ".dds" || s == ".png" || s == ".jpg" || s == ".tga" || s == ".bmp" || s == ".hdr";
}

bool IsMaterialFileExtension(const std::string& strExtension)
{
	std::string s = s2lower(strExtension);
	return s == ".nmat";
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
