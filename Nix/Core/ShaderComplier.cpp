#include "ShaderComplier.h"
#include "BaseDefs/NixCore.h"
#include "NXGlobalDefinitions.h"
#include "NXShaderIncluder.h"

const std::wstring NXShaderComplier::s_smVersionVS = L"vs_6_8";
const std::wstring NXShaderComplier::s_smVersionPS = L"ps_6_8";
const std::wstring NXShaderComplier::s_smVersionCS = L"cs_6_8";

NXShaderComplier::NXShaderComplier() :
	m_shaderFlags2(0),
	m_pd3dInclude(new NXFullyIncludeHandler())
{
	m_shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	m_shaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	m_shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

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

	return CompileInternal(sourceBuffer, shaderFilePath.wstring(), mainFuncEntryPoint, pVSBlob, oErrorMessage, clearDefineMacros);
}

HRESULT NXShaderComplier::CompilePS(const std::filesystem::path& shaderFilePath, const std::wstring& mainFuncEntryPoint, IDxcBlob** pPSBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	ComPtr<IDxcBlobEncoding> pSrcBlob;
	m_pDXCUtils->LoadFile(shaderFilePath.c_str(), nullptr, &pSrcBlob);

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = pSrcBlob->GetBufferPointer();
	sourceBuffer.Size = pSrcBlob->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	return CompileInternal(sourceBuffer, shaderFilePath.wstring(), mainFuncEntryPoint, pPSBlob, oErrorMessage, clearDefineMacros);
}
HRESULT NXShaderComplier::CompileVSByCode(const std::string& shaderCode, const std::wstring& mainFuncEntryPoint, IDxcBlob** pVSBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = shaderCode.data();
	sourceBuffer.Size = shaderCode.size();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	return CompileInternal(sourceBuffer, L"[Custom Shader]", mainFuncEntryPoint, pVSBlob, oErrorMessage, clearDefineMacros);
}

HRESULT NXShaderComplier::CompilePSByCode(const std::string& shaderCode, const std::wstring& mainFuncEntryPoint, IDxcBlob** pPSBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = shaderCode.data();
	sourceBuffer.Size = shaderCode.size();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	return CompileInternal(sourceBuffer, L"[Custom Shader]", mainFuncEntryPoint, pPSBlob, oErrorMessage, clearDefineMacros);
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
	SafeDelete(m_pd3dInclude);
}

HRESULT NXShaderComplier::CompileInternal(const DxcBuffer& sourceBuffer, const std::wstring& shaderName, const std::wstring& mainFuncEntryPoint, IDxcBlob** pBlob, std::string& oErrorMessage, bool clearDefineMacros)
{
	// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
	std::vector<LPCWSTR> args =
	{
		shaderName.c_str(),					// Optional shader source file name for error reporting and for PIX shader source view.  
		L"-E", mainFuncEntryPoint.c_str(),  // Entry point.
		L"-T", s_smVersionVS.c_str(),       // Target.
		L"-Zi"								// Enable debug information
	};

	for (auto& def : m_defineMacros)
	{
		args.push_back(L"-D");
		std::wstring str = def.name + L"=" + def.value;
		args.push_back(str.c_str());
	}

	// 按照上面的参数编译shader
	ComPtr<IDxcResult> pResult;
	HRESULT hr = m_pDXCCompiler->Compile(&sourceBuffer, args.data(), (UINT)args.size(), m_pDXCIncHandler.Get(), IID_PPV_ARGS(&pResult));
	if (FAILED(hr))
	{
		std::wstring msg = L"shader " + shaderName + L" cannot be compiled.";
		MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
		return hr;
	}

	// 如果有Error，输出错误信息
	ComPtr<IDxcBlobUtf8> pErrors = nullptr;
	hr = pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
	if (pErrors != nullptr && pErrors->GetStringLength() != 0)
	{
		oErrorMessage = reinterpret_cast<const char*>(pErrors->GetBufferPointer());
		OutputDebugStringA(oErrorMessage.c_str());

		std::wstring msg = L"shader " + shaderName + L" cannot be compiled.";
		MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
	}

	HRESULT hrStatus;
	pResult->GetStatus(&hrStatus);
	if (FAILED(hrStatus))
	{
		std::wstring msg = L"shader " + shaderName + L" cannot be compiled.";
		MessageBox(nullptr, msg.c_str(), L"error", MB_OK);
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
