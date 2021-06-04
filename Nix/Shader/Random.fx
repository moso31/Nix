// 伪随机梯度噪声，范围[0, 1)
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
float InterleavedGradientNoise(float2 value, int offset)
{
	const float3 magic = float3(0.06711056f * 100, 0.00583715f * 100, 52.9829189f);
	float2 scale = float2(1.114514f, 2.1919810f);	// scale不要用const，不然offset=0的时候编译器优化不掉
	value += offset * scale;
	return frac(magic.z * frac(dot(value, magic.xy)));
}
