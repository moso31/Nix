struct CBufferSector2VirtImg
{
	// 哪个像素
    int2 sectorPos;
    
	// 改成什么值
    int indiTexData; // x(12bit)y(12bit) = indi tex pos; z(8bit) = indi tex size
    int _0;
};

#define cbSec2VirtImgUpdateDataMaxLimit 256

cbuffer cbSec2VirtImgUpdateData : register(b0)
{
    CBufferSector2VirtImg m_cbData[cbSec2VirtImgUpdateDataMaxLimit];
}

cbuffer cbSec2VirtImgUpdateDataNum : register(b1)
{
    int m_cbDataNum;
    int3 _1;
}

RWTexture2D<uint> m_VTSector2VirtImg : register(u0);

[numthreads(8, 1, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= m_cbDataNum)
        return;
    
    m_VTSector2VirtImg[m_cbData[dtid.x].sectorPos] = m_cbData[dtid.x].indiTexData;
}
