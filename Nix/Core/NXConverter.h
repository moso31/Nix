#pragma once
#include "BaseDefs/CppSTLFully.h"
#include "BaseDefs/DX12.h"
#include "NXConvertString.h"

namespace NXConvert
{

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

// 2024.4.9
// ��һ��DXGI_FORMAT��ȥ��Typeless����
// DX12���鴴����Դʱָ��clearValue����clearValue�ĸ�ʽ������Typeless����
DXGI_FORMAT DXGINoTypeless(DXGI_FORMAT fmt, bool isDepthStencil = false);

// 2024.6.22
// ��һ��fmt��ʽת����DSVFormat����ʹ�õĸ�ʽ
// ���fmt����DepthStencil֧�ֵ����ͣ�����DXGI_FORMAT_UNKNOWN
DXGI_FORMAT TypelessToDSVFormat(DXGI_FORMAT fmt);

DXGI_FORMAT ForceSRGB(DXGI_FORMAT fmt);
DXGI_FORMAT ForceLinear(DXGI_FORMAT fmt);

bool IsImageFileExtension(const std::string& strExt);
bool IsMaterialFileExtension(const std::string& strExt);
bool IsDiffuseProfileExtension(const std::string& strExt);

bool IsMaterialDefaultPath(const std::string& s);

void GetMipSliceFromLayoutIndex(int layoutIndex, int mipSize, int sliceSize, int& oMip, int& oSlice);

}