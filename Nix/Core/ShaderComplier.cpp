#include "ShaderComplier.h"
#include "NXShaderIncluder.h"

const std::string NXShaderComplier::s_smVersionVS = "vs_5_0";
const std::string NXShaderComplier::s_smVersionPS = "ps_5_0";
const std::string NXShaderComplier::s_smVersionCS = "cs_5_0";

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
}

NXShaderComplier::~NXShaderComplier()
{
}

HRESULT NXShaderComplier::CompileVS(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11VertexShader** ppOutVS, bool clearDefineMacros)
{
	ComPtr<ID3DBlob> pBlob;
	HRESULT hr = S_OK;

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(shaderFilePath.c_str(), m_defineMacros.data(), m_pd3dInclude, mainFuncEntryPoint.c_str(), s_smVersionVS.c_str(),
		m_shaderFlags, m_shaderFlags2, &pBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	std::wstring errorMessage = L"shader " + shaderFilePath + L" cannot be compiled.  Please run this executable from the directory that contains the FX file.";
	NX::MessageBoxIfFailed(hr, errorMessage.c_str());

	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppOutVS));

	if (clearDefineMacros) ClearMacros();

	return S_OK;
}

HRESULT NXShaderComplier::CompileVSIL(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11VertexShader** ppOutVS, const D3D11_INPUT_ELEMENT_DESC* pILDescs, UINT numElementsOfIL, ID3D11InputLayout** ppOutIL, bool clearDefineMacros)
{
	ComPtr<ID3DBlob> pBlob;
	HRESULT hr = S_OK;

	if (!m_defineMacros.empty())
	{
		// 补一个空 shader macro, 否则会报错
		m_defineMacros.push_back(CD3D_SHADER_MACRO(nullptr, nullptr));
	}

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(shaderFilePath.c_str(), m_defineMacros.data(), m_pd3dInclude, mainFuncEntryPoint.c_str(), s_smVersionVS.c_str(),
		m_shaderFlags, m_shaderFlags2, &pBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	std::wstring errorMessage = L"shader " + shaderFilePath + L" cannot be compiled.  Please run this executable from the directory that contains the FX file.";
	NX::MessageBoxIfFailed(hr, errorMessage.c_str());

	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppOutVS));

	if (clearDefineMacros) ClearMacros();

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(pILDescs, numElementsOfIL, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), ppOutIL));

	return S_OK;
}

HRESULT NXShaderComplier::CompileVSILByCode(const std::string& shaderCode, std::string mainFuncEntryPoint, ID3D11VertexShader** ppOutVS, const D3D11_INPUT_ELEMENT_DESC* pILDescs, UINT numElementsOfIL, ID3D11InputLayout** ppOutIL, bool clearDefineMacros)
{
	ComPtr<ID3DBlob> pBlob;
	HRESULT hr = S_OK;

	if (!m_defineMacros.empty())
	{
		// 补一个空 shader macro, 否则会报错
		m_defineMacros.push_back(CD3D_SHADER_MACRO(nullptr, nullptr));
	}

	ComPtr<ID3DBlob> pErrorBlob;
	hr = D3DCompile(shaderCode.c_str(), shaderCode.size(), nullptr, m_defineMacros.data(), m_pd3dInclude, mainFuncEntryPoint.c_str(), s_smVersionVS.c_str(),
		m_shaderFlags, m_shaderFlags2, &pBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob) OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
		return hr;
	}

	std::wstring errorMessage = L"shader [CUSTOMCODE] cannot be compiled.  Please run this executable from the directory that contains the FX file.";
	NX::MessageBoxIfFailed(hr, errorMessage.c_str());

	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppOutVS));

	if (clearDefineMacros) ClearMacros();

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(pILDescs, numElementsOfIL, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), ppOutIL));

	return S_OK;
}

HRESULT NXShaderComplier::CompilePS(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11PixelShader** ppOutPS, bool clearDefineMacros)
{
	ComPtr<ID3DBlob> pBlob;
	HRESULT hr = S_OK;

	if (!m_defineMacros.empty())
	{
		// 补一个空 shader macro, 否则会报错
		m_defineMacros.push_back(CD3D_SHADER_MACRO(nullptr, nullptr));
	}

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(shaderFilePath.c_str(), m_defineMacros.data(), m_pd3dInclude, mainFuncEntryPoint.c_str(), s_smVersionPS.c_str(),
		m_shaderFlags, m_shaderFlags2, &pBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	std::wstring errorMessage = L"shader " + shaderFilePath + L" cannot be compiled.  Please run this executable from the directory that contains the FX file.";
	NX::MessageBoxIfFailed(hr, errorMessage.c_str());

	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppOutPS));

	if (clearDefineMacros) ClearMacros();

	return S_OK;
}

HRESULT NXShaderComplier::CompilePSByCode(const std::string& shaderCode, std::string mainFuncEntryPoint, ID3D11PixelShader** ppOutPS, bool clearDefineMacros)
{
	ComPtr<ID3DBlob> pBlob;
	HRESULT hr = S_OK;

	if (!m_defineMacros.empty())
	{
		// 补一个空 shader macro, 否则会报错
		m_defineMacros.push_back(CD3D_SHADER_MACRO(nullptr, nullptr));
	}

	ComPtr<ID3DBlob> pErrorBlob;
	hr = D3DCompile(shaderCode.c_str(), shaderCode.size(), nullptr, m_defineMacros.data(), m_pd3dInclude, mainFuncEntryPoint.c_str(), s_smVersionPS.c_str(),
		m_shaderFlags, m_shaderFlags2, &pBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob) OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
		return hr;
	}

	std::wstring errorMessage = L"shader [CUSTOMCODE] cannot be compiled.  Please run this executable from the directory that contains the FX file.";
	NX::MessageBoxIfFailed(hr, errorMessage.c_str());

	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppOutPS));

	if (clearDefineMacros) ClearMacros();

	return S_OK;
}

HRESULT NXShaderComplier::CompileCS(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11ComputeShader** ppOutCS, bool clearDefineMacros)
{
	ComPtr<ID3DBlob> pBlob;
	HRESULT hr = S_OK;

	if (!m_defineMacros.empty()) 
	{ 
		// 补一个空 shader macro, 否则会报错
		m_defineMacros.push_back(CD3D_SHADER_MACRO(nullptr, nullptr)); 
	}

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(shaderFilePath.c_str(), m_defineMacros.data(), m_pd3dInclude, mainFuncEntryPoint.c_str(), s_smVersionCS.c_str(),
		m_shaderFlags, m_shaderFlags2, &pBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	std::wstring errorMessage = L"shader " + shaderFilePath + L" cannot be compiled.  Please run this executable from the directory that contains the FX file.";
	NX::MessageBoxIfFailed(hr, errorMessage.c_str());

	NX::ThrowIfFailed(g_pDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppOutCS));

	if (clearDefineMacros) ClearMacros();

	return S_OK;
}

void NXShaderComplier::AddMacro(const CD3D_SHADER_MACRO& macro)
{
	m_defineMacros.push_back(macro);
}

void NXShaderComplier::ClearMacros()
{
	m_defineMacros.clear();
}
