#pragma once
#include "NXRGPass.h"
#include "NXRGResource.h"
#include "NXReadbackData.h"

class NXReadbackBufferPass : public NXRGPass
{
public:
	NXReadbackBufferPass();
	virtual ~NXReadbackBufferPass() {}

	virtual void SetupInternal() override {}
	virtual void Render() override;

	void SetInput(NXRGResource* pRes) { m_pReadbackBuffer = pRes; }
	void SetOutput(Ntr<NXReadbackData>& pOutData) { m_pOutData = pOutData; }

	void AdjustOutputDataSize();

private:
	NXRGResource* m_pReadbackBuffer; // input
	Ntr<NXReadbackData> m_pOutData; // output
};