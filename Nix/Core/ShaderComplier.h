#pragma once
#include "Header.h"
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
	~CD3D_SHADER_MACRO() {}
	operator const D3D_SHADER_MACRO& () const { return *this; }
};

class NXShaderComplier : public NXInstance<NXShaderComplier>
{
public:
	NXShaderComplier();
	~NXShaderComplier();

	HRESULT CompileVS(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11VertexShader** ppOutVS, bool clearDefineMacros = true);
	HRESULT CompileVSIL(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11VertexShader** ppOutVS, const D3D11_INPUT_ELEMENT_DESC* pILDescs, UINT numElementsOfIL, ID3D11InputLayout** ppOutIL, bool clearDefineMacros = true);
	HRESULT CompileVSILByCode(const std::string& shaderCode, std::string mainFuncEntryPoint, ID3D11VertexShader** ppOutVS, const D3D11_INPUT_ELEMENT_DESC* pILDescs, UINT numElementsOfIL, ID3D11InputLayout** ppOutIL, bool clearDefineMacros = true);
	HRESULT CompilePS(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11PixelShader** ppOutPS, bool clearDefineMacros = true);
	HRESULT CompilePSByCode(const std::string& shaderCode, std::string mainFuncEntryPoint, ID3D11PixelShader** ppOutPS, bool clearDefineMacros = true);
	HRESULT CompileCS(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11ComputeShader** ppOutCS, bool clearDefineMacros = true);

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