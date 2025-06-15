ConsumeStructuredBuffer<uint2> m_inBuffer : register(u0);
AppendStructuredBuffer<uint2> m_outBuffer : register(u1);
AppendStructuredBuffer<uint2> m_final : register(u2);

[numthreads(2, 2, 1)]
void CS_Pass(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 dtid = dispatchThreadID.xy;
    
    m_outBuffer.Append(dtid.xy);
}
