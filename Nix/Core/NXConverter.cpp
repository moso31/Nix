#include "NXConverter.h"

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
	std::transform(s.begin(), s.end(), s.begin(), [](UCHAR c) { return std::tolower(c); });
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

}
