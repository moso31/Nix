RWStructuredBuffer<uint> gOutput : register(u0);

[numthreads(64, 1, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    // ȷ����Խ��
    if (index < 256)
    {
        gOutput[index] = 42; // ��ÿ��Ԫ��д��42
    }
}
