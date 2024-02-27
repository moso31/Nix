#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/CppSTLFully.h"

class NXFullyIncludeHandler : public ID3DInclude
{
public:
    STDMETHOD(Open)(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
    {
        std::string filePath = "./Shader/" + std::string(pFileName);
        std::ifstream fileStream(filePath, std::ios::binary | std::ios::ate);

        if (!fileStream.is_open())
        {
            return E_FAIL;
        }

        std::streampos fileSize = fileStream.tellg();
        char* buffer = new char[fileSize];

        fileStream.seekg(0, std::ios::beg);
        fileStream.read(buffer, fileSize);
        fileStream.close();

        *ppData = buffer;
        *pBytes = static_cast<UINT>(fileSize);

        return S_OK;
    }

    STDMETHOD(Close)(LPCVOID pData)
    {
        char* buffer = (char*)pData;
        delete[] buffer;
        return S_OK;
    }

    STDMETHOD_(ULONG, AddRef)() { return 1; }
    STDMETHOD_(ULONG, Release)() { return 1; }

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj)
    {
        if (ppvObj == nullptr)
        {
            return E_INVALIDARG;
        }
        if (riid == __uuidof(IUnknown))
        {
            *ppvObj = this;
            return S_OK;
        }
        *ppvObj = nullptr;
        return E_NOINTERFACE;
    }
};