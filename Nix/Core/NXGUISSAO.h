#pragma once

class NXSimpleSSAO;
class NXGUISSAO
{
public:
	NXGUISSAO(NXSimpleSSAO* pSSAO);
	~NXGUISSAO() {}

	void Render();

private:
	NXSimpleSSAO* m_pSSAO;

	bool m_bDirty;
};