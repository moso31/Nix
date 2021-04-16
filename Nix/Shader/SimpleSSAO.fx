SamplerState SamplerStateTrilinear : register(s0);

Texture2D txNormal : register(t0);
Texture2D txDepthZ : register(t1);

RWTexture2D<float> txSimpleSSAO : register(u0);

[numthreads(8, 8, 1)]
void CS(int3 DTid : SV_DispatchThreadID)
{
	float Normal = txNormal.Load(DTid).z;
	float Depth = txDepthZ.Load(DTid).x;
	txSimpleSSAO[DTid.xy] = Normal * Depth;
}