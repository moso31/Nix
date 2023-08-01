#pragma once
#include "Header.h"

namespace NXConvert
{

//  wstring -> string 
std::string ws2s(const std::wstring& ws);

// string -> wstring
std::wstring s2ws(const std::string& s);

std::string s2lower(const std::string& s);
std::wstring s2lower(const std::wstring& s);

std::string Trim(std::string& str);

std::string GetPathOfImguiIni();

// ��һ�κ����ı�����ȡ���ú��������С������Ҫ������ HLSL �����
std::string GetTitleOfFunctionData(const std::string& functionString);

// 2023.5.31
// ����Ƿ��� UNORM ��ʽ������
bool IsUnormFormat(DXGI_FORMAT fmt);

// 2023.5.31
// �Զ���� UNORM ��ʽ���������а�ȫ����ʽת����
// UNORM ���͵�������Ҫ�������¹�����д���
// 1. ��R, RG, RGB��ʽ��UNORM����ת��8λ��RGBA_UNORM;
// 2. �Է�8λ��UNORM����ת��8λ��RGBA_UNORM;
// 3. ����SRGBβ׺
// ��������������ʱ����Ҫ���д���
DXGI_FORMAT SafeDXGIFormat(DXGI_FORMAT fmt);

DXGI_FORMAT ForceSRGB(DXGI_FORMAT fmt);
DXGI_FORMAT ForceLinear(DXGI_FORMAT fmt);

bool IsImageFileExtension(const std::string& strFilePath);

bool IsMaterialFileExtension(const std::string& strFilePath);

void getline_safe(std::ifstream& ifs, std::string& s);

bool IsMaterialDefaultPath(const std::string& s);

}