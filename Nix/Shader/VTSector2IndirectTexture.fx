struct CBufferSector2IndirectTexture
{
	// 哪个像素
    int2 sectorPos;
    
	// 改成什么值
    int indiTexData; // x(12bit)y(12bit) = indi tex pos; z(8bit) = indi tex size
    int _0;
};

#define cbSec2IndiTexUpdateDataNum 256

cbuffer cbSec2IndiTexUpdateData : register(b0)
{
    CBufferSector2IndirectTexture m_cbData[cbSec2IndiTexUpdateDataNum];
}

RWTexture2D<uint> txSector2IndirectTexture : register(u0);

[numthreads(8, 1, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    txSector2IndirectTexture[m_cbData[dtid.x].sectorPos] = m_cbData[dtid.x].indiTexData;
}
