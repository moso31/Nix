#pragma once
#include <string>
#include <filesystem>
#include "BaseDefs/DX12.h"
#include "NXInstance.h"

struct NXD3D_SHADER_MACRO
{
	std::wstring name;
	std::wstring value;
};

class NXShaderComplier : public NXInstance<NXShaderComplier>
{
public:
	NXShaderComplier();
	virtual ~NXShaderComplier();

	HRESULT CompileVS(const std::filesystem::path& shaderFilePath, const std::wstring& mainFuncEntryPoint, IDxcBlob** pVSBlob, std::string& oErrorMessage = std::string(), bool clearDefineMacros = true);
	HRESULT CompilePS(const std::filesystem::path& shaderFilePath, const std::wstring& mainFuncEntryPoint, IDxcBlob** pPSBlob, std::string& oErrorMessage = std::string(), bool clearDefineMacros = true);

	HRESULT CompileVSByCode(const std::string& shaderCode, const std::wstring& mainFuncEntryPoint, IDxcBlob** pVSBlob, std::string& oErrorMessage = std::string(), bool clearDefineMacros = true);
	HRESULT CompilePSByCode(const std::string& shaderCode, const std::wstring& mainFuncEntryPoint, IDxcBlob** pPSBlob, std::string& oErrorMessage = std::string(), bool clearDefineMacros = true);

	void AddMacro(const std::wstring& name, const std::wstring& value);
	void ClearMacros();

	void Release();

private:
	HRESULT CompileInternal(const DxcBuffer& sourceBuffer, const std::wstring& shaderName, const std::wstring& mainFuncEntryPoint, IDxcBlob** pBlob, std::string& oErrorMessage, bool clearDefineMacros);

private:
	std::vector<NXD3D_SHADER_MACRO> m_defineMacros;

	ID3DInclude* m_pd3dInclude;
	DWORD m_shaderFlags;
	DWORD m_shaderFlags2;

	static const std::wstring s_smVersionVS;
	static const std::wstring s_smVersionPS;
	static const std::wstring s_smVersionCS;

	ComPtr<IDxcUtils> m_pDXCUtils;
	ComPtr<IDxcCompiler3> m_pDXCCompiler;
	ComPtr<IDxcIncludeHandler> m_pDXCIncHandler;
};