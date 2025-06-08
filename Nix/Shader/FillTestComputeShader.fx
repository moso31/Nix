ConsumeStructuredBuffer<uint2> m_inBuffer : register(u0);
AppendStructuredBuffer<uint2> m_outBuffer : register(u1);
AppendStructuredBuffer<uint2> m_final : register(u2);

[numthreads(8, 8, 1)]
void CS_PrePass(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    m_outBuffer[index] = m_outBuffer[index];
}

[numthreads(64, 1, 1)]
void CS_Pass(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    m_outBuffer[index] = m_outBuffer[index];
}
