#pragma once
#include <string>
#include <filesystem>
#include "BaseDefs/DX12.h"
#include "NXInstance.h"

struct CD3D_SHADER_MACRO : public D3D_SHADER_MACRO
{
	CD3D_SHADER_MACRO() = default;
	explicit CD3D_SHADER_MACRO(const D3D_SHADER_MACRO& macro) : D3D_SHADER_MACRO(macro) {}
	explicit CD3D_SHADER_MACRO(const LPCSTR name, const LPCSTR definition)
	{
		Name = name;
		Definition = definition;
	}
	virtual ~CD3D_SHADER_MACRO() {}
	operator const D3D_SHADER_MACRO& () const { return *this; }
};

class NXShaderComplier : public NXInstance<NXShaderComplier>
{
public:
	NXShaderComplier();
	virtual ~NXShaderComplier();

	HRESULT CompileVS(const std::filesystem::path& shaderFilePath, const std::string& mainFuncEntryPoint, ID3DBlob** pVSBlob, std::string& oErrorMessage = std::string(), bool clearDefineMacros = true);
	HRESULT CompilePS(const std::filesystem::path& shaderFilePath, const std::string& mainFuncEntryPoint, ID3DBlob** pPSBlob, std::string& oErrorMessage = std::string(), bool clearDefineMacros = true);

	HRESULT CompileVSByCode(const std::string& shaderCode, const std::string& mainFuncEntryPoint, ID3DBlob** pVSBlob, std::string& oErrorMessage = std::string(), bool clearDefineMacros = true);
	HRESULT CompilePSByCode(const std::string& shaderCode, const std::string& mainFuncEntryPoint, ID3DBlob** pPSBlob, std::string& oErrorMessage = std::string(), bool clearDefineMacros = true);

	void AddMacro(const CD3D_SHADER_MACRO& macro);
	void ClearMacros();

	void Release();

private:
	std::vector<CD3D_SHADER_MACRO> m_defineMacros;

	ID3DInclude* m_pd3dInclude;
	DWORD m_shaderFlags;
	DWORD m_shaderFlags2;

	static const std::string s_smVersionVS;
	static const std::string s_smVersionPS;
	static const std::string s_smVersionCS;
};