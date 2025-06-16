ConsumeStructuredBuffer<uint2> m_inBuffer : register(u0);
AppendStructuredBuffer<uint2> m_outBuffer : register(u1);
AppendStructuredBuffer<uint2> m_final : register(u2);

[numthreads(1, 1, 1)]
void CS_Pass(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 dtid = dispatchThreadID.xy;
    
    uint2 c = m_inBuffer.Consume();
    m_outBuffer.Append(c * uint2(10,10) + uint2(0,0));
    m_outBuffer.Append(c * uint2(10,10) + uint2(0,1));
    m_outBuffer.Append(c * uint2(10,10) + uint2(1,0));
    m_outBuffer.Append(c * uint2(10,10) + uint2(0,1));
}
