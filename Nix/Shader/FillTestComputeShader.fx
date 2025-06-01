RWStructuredBuffer<uint> gOutput : register(u0);

[numthreads(64, 1, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    // 确保不越界
    if (index < 256)
    {
        gOutput[index] = 42; // 将每个元素写成42
    }
}
