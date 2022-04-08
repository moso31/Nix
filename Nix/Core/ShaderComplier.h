#pragma once
#include "Header.h"
#include "NXInstance.h"

class NXShaderComplier : public NXInstance<NXShaderComplier>
{
public:
	NXShaderComplier();
	~NXShaderComplier();

	HRESULT CompileVS(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11VertexShader** ppOutVS, bool clearDefineMacros = true);
	HRESULT CompileVSIL(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11VertexShader** ppOutVS, const D3D11_INPUT_ELEMENT_DESC* pILDescs, UINT numElementsOfIL, ID3D11InputLayout** ppOutIL, bool clearDefineMacros = true);
	HRESULT CompilePS(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11PixelShader** ppOutPS, bool clearDefineMacros = true);
	HRESULT CompileCS(std::wstring shaderFilePath, std::string mainFuncEntryPoint, ID3D11ComputeShader** ppOutCS, bool clearDefineMacros = true);

	void AddMacro(const D3D_SHADER_MACRO& macro);
	void ClearMacros();

private:
	std::vector<D3D_SHADER_MACRO> m_defineMacros;

	ID3DInclude* m_pd3dInclude;
	DWORD m_shaderFlags;
	DWORD m_shaderFlags2;

	static const std::string s_smVersionVS;
	static const std::string s_smVersionPS;
	static const std::string s_smVersionCS;
};