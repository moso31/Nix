ConsumeStructuredBuffer<uint2> m_inBuffer : register(u0);
AppendStructuredBuffer<uint2> m_outBuffer : register(u1);
AppendStructuredBuffer<uint2> m_final : register(u2);

cbuffer m_terrainParam : register(b0)
{
    float3 camPos;
    float scale;
    uint x;
}

[numthreads(1, 1, 1)]
void CS_Pass(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 dtid = dispatchThreadID.xy;
    
    uint2 c = m_inBuffer.Consume();
    c += frac(camPos.x);
    m_outBuffer.Append(c * uint2(10,10) + uint2(0,0));
    m_outBuffer.Append(c * uint2(10,10) + uint2(0,1));
    m_outBuffer.Append(c * uint2(10,10) + uint2(1,0));
    m_outBuffer.Append(c * uint2(10,10) + uint2(0,1));
}
