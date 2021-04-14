Texture2D txDepthZ : register(t0);
Texture2D txNormal : register(t1);
RWTexture2D<float> txSimpleSSAO : register(u0);

[numthreads(8, 8, 1)]
void CS(int3 DTid : SV_DispatchThreadID)
{
	txSimpleSSAO[DTid.xy] = DTid.x;
}