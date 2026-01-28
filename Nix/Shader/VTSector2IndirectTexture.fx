struct CBufferSector2IndirectTexture
{
	// 哪个像素
    int2 sectorPos;
    
	// 改成什么值
    int indiTexData; // x(12bit)y(12bit) = indi tex pos; z(8bit) = indi tex size
    int _0;
};

#define cbSec2IndiTexUpdateDataMaxLimit 256

cbuffer cbSec2IndiTexUpdateData : register(b0)
{
    CBufferSector2IndirectTexture m_cbData[cbSec2IndiTexUpdateDataMaxLimit];
}

cbuffer cbSec2IndiTexUpdateDataNum : register(b1)
{
    int m_cbDataNum;
    int3 _1;
}

RWTexture2D<uint> txSector2IndirectTexture : register(u0);

[numthreads(8, 1, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= m_cbDataNum)
        return;
    
    txSector2IndirectTexture[m_cbData[dtid.x].sectorPos] = m_cbData[dtid.x].indiTexData;
}
