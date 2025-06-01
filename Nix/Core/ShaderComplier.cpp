#include "ShaderComplier.h"
#include "BaseDefs/NixCore.h"
#include "NXGlobalDefinitions.h"

const std::wstring NXShaderComplier::s_smVersionVS = L"vs_6_5";
const std::wstring NXShaderComplier::s_smVersionPS = L"ps_6_5";
const std::wstring NXShaderComplier::s_smVersionCS = L"cs_6_5";

NXShaderComplier::NXShaderComplier() 
{
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_pDXCUtils));
	if (FAILED(hr))
	{
		std::wstring msg = L"DXCUtils create failed";
		MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
	}

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pDXCCompiler));
	if (FAILED(hr))
	{
		std::wstring msg = L"DXCCompiler create failed";
		MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
	}

	hr = m_pDXCUtils->CreateDefaultIncludeHandler(&m_pDXCIncHandler);
	if (FAILED(hr))
	{
		std::wstring msg = L"DXCIncludeHandler create failed";
		MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
	}
}

NXShaderComplier::~NXShaderComplier()
{
}

HRESULT NXShaderComplier::CompileVS(const std::filesystem::path& shaderFilePath, const std::wstring& mainFuncEntryPoint, IDxcBlob** pVSBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	ComPtr<IDxcBlobEncoding> pSrcBlob;
	HRESULT hr = m_pDXCUtils->LoadFile(shaderFilePath.c_str(), nullptr, &pSrcBlob);
	if (FAILED(hr))
	{
		std::wstring msg = L"shader " + shaderFilePath.wstring() + L" cannot be loaded.  Please run this executable from the directory that contains the FX file.";
		MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
		return hr;
	}

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = pSrcBlob->GetBufferPointer();
	sourceBuffer.Size = pSrcBlob->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	return CompileInternal(sourceBuffer, shaderFilePath.wstring(), s_smVersionVS, mainFuncEntryPoint, pVSBlob, oErrorMessage, clearDefineMacros);
}

HRESULT NXShaderComplier::CompilePS(const std::filesystem::path& shaderFilePath, const std::wstring& mainFuncEntryPoint, IDxcBlob** pPSBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	ComPtr<IDxcBlobEncoding> pSrcBlob;
	m_pDXCUtils->LoadFile(shaderFilePath.c_str(), nullptr, &pSrcBlob);

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = pSrcBlob->GetBufferPointer();
	sourceBuffer.Size = pSrcBlob->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	return CompileInternal(sourceBuffer, shaderFilePath.wstring(), s_smVersionPS, mainFuncEntryPoint, pPSBlob, oErrorMessage, clearDefineMacros);
}
HRESULT NXShaderComplier::CompileCS(const std::filesystem::path& shaderFilePath, const std::wstring& mainFuncEntryPoint, IDxcBlob** pCSBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	ComPtr<IDxcBlobEncoding> pSrcBlob;
	HRESULT hr = m_pDXCUtils->LoadFile(shaderFilePath.c_str(), nullptr, &pSrcBlob);
	if (FAILED(hr))
	{
		std::wstring msg = L"shader " + shaderFilePath.wstring() + L" cannot be loaded.  Please run this executable from the directory that contains the FX file.";
		MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
		return hr;
	}

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = pSrcBlob->GetBufferPointer();
	sourceBuffer.Size = pSrcBlob->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	return CompileInternal(
		sourceBuffer,
		shaderFilePath.wstring(),
		s_smVersionCS,
		mainFuncEntryPoint,
		pCSBlob,
		oErrorMessage,
		clearDefineMacros);
}
HRESULT NXShaderComplier::CompileVSByCode(const std::string& shaderCode, const std::wstring& mainFuncEntryPoint, IDxcBlob** pVSBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = shaderCode.data();
	sourceBuffer.Size = shaderCode.size();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	return CompileInternal(sourceBuffer, L"[Custom Shader]", s_smVersionVS, mainFuncEntryPoint, pVSBlob, oErrorMessage, clearDefineMacros);
}

HRESULT NXShaderComplier::CompilePSByCode(const std::string& shaderCode, const std::wstring& mainFuncEntryPoint, IDxcBlob** pPSBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = shaderCode.data();
	sourceBuffer.Size = shaderCode.size();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	return CompileInternal(sourceBuffer, L"[Custom Shader]", s_smVersionPS, mainFuncEntryPoint, pPSBlob, oErrorMessage, clearDefineMacros);
}

HRESULT NXShaderComplier::CompileCSByCode(const std::string& shaderCode, const std::wstring& mainFuncEntryPoint, IDxcBlob** pCSBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = shaderCode.data();
	sourceBuffer.Size = shaderCode.size();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	return CompileInternal(sourceBuffer, L"[Custom Shader]", s_smVersionCS, mainFuncEntryPoint, pCSBlob, oErrorMessage, clearDefineMacros);
}

void NXShaderComplier::AddMacro(const std::wstring& name, const std::wstring& value)
{
	m_defineMacros.push_back({ name, value });
}

void NXShaderComplier::ClearMacros()
{
	m_defineMacros.clear();
}

void NXShaderComplier::Release()
{
}

HRESULT NXShaderComplier::CompileInternal(const DxcBuffer& sourceBuffer, const std::wstring& shaderName, const std::wstring& smVersion, const std::wstring& mainFuncEntryPoint, IDxcBlob** pBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
	std::vector<std::wstring> macroDefines;
	std::vector<LPCWSTR> args =
	{
		shaderName.c_str(),					// Optional shader source file name for error reporting and for PIX shader source view.  
		L"-E", mainFuncEntryPoint.c_str(),  // Entry point.
		L"-T", smVersion.c_str(),			// Target.
		L"-Zi"								// Enable debug information
	};

	for (auto& def : m_defineMacros)
	{
		args.push_back(L"-D");
		macroDefines.push_back(def.name + L"=" + def.value);
		args.push_back(macroDefines.back().c_str());
	}

	// Include path
	args.push_back(L"-I");
	args.push_back(L"./Shader/");

#ifdef DEBUG
	// Debug mode
	args.push_back(L"-Qembed_debug");
	args.push_back(L"-Qsource_in_debug_module");
#endif

	// 按照上面的参数编译shader
	ComPtr<IDxcResult> pResult;
	HRESULT hr = m_pDXCCompiler->Compile(&sourceBuffer, args.data(), (UINT)args.size(), m_pDXCIncHandler.Get(), IID_PPV_ARGS(&pResult));
	if (FAILED(hr))
	{
		std::wstring msg = L"shader " + shaderName + L" cannot be compiled.";
		MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
		return hr;
	}

	HRESULT hrStatus;
	pResult->GetStatus(&hrStatus);
	if (FAILED(hrStatus))
	{
		ComPtr<IDxcBlobUtf8> pErrors = nullptr;
		hr = pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
		if (pErrors != nullptr && pErrors->GetStringLength() != 0)
		{
			oErrorMessage = reinterpret_cast<const char*>(pErrors->GetBufferPointer());
			OutputDebugStringA(oErrorMessage.c_str());

			std::wstring msg = L"shader " + shaderName + L" cannot be compiled.";
			MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
		}

		return hrStatus;
	}

	// 获取编译结果blob
	ComPtr<IDxcBlobUtf16> pShaderName = nullptr;
	ComPtr<IDxcBlob> pShaderBlob = nullptr;
	hr = pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShaderBlob), &pShaderName);
	if (FAILED(hr))
	{
		std::wstring msg = L"shader " + shaderName + L" cannot be compiled.";
		MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
		return hr;
	}
	*pBlob = pShaderBlob.Detach();

	if (clearDefineMacros) ClearMacros();

	return S_OK;
}
