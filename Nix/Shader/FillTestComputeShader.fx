ConsumeStructuredBuffer<uint2> m_inBuffer : register(u0);
AppendStructuredBuffer<uint2> m_outBuffer : register(u1);
AppendStructuredBuffer<uint2> m_final : register(u2);

[numthreads(64, 1, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    gOutput[index] = gOutput[index];
}
