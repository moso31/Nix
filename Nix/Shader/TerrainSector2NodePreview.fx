#define MipCount 6

cbuffer cbRemapParams : register(b0)
{
    float m_remapMin;
    float m_remapMax;
    
    int2 pad0;
};

cbuffer cbMip : register(b1)
{
    int m_currentMip;
    
    int3 pad1;
}

Texture2D<uint> txSector2NodeID : register(t0); 
RWTexture2D<float3> txPreview : register(u0); 

[numthreads(8, 8, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    uint2 pixelPos = dtid.xy;
    uint width, height;
    txPreview.GetDimensions(width, height);
    
    if (pixelPos.x >= width || pixelPos.y >= height)
        return;
    
    // 读uint16纹理
    uint nodeID = txSector2NodeID.Load(int3(pixelPos, m_currentMip));
    
    float normalizedValue;
    if (nodeID == 0xFFFF) // -1是无效像素
    {
        txPreview[pixelPos] = float3(1.0, 0.5, 0.5);
        return;
    }
    else
    {
        float range = max(m_remapMax - m_remapMin, 1.0f);
        normalizedValue = saturate((float(nodeID) - m_remapMin) / range);
    }
    
    txPreview[pixelPos] = float3(normalizedValue, normalizedValue, normalizedValue);
}
