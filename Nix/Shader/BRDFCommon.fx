struct BSDFValue
{
	float3 Diffuse;
	float3 Specular;
};

const static float NX_PIDIV2 = 1.5707963267948966192313216916398f;
const static float NX_PI = 3.1415926535897932384626433832795f;
const static float NX_2PI = 6.283185307179586476925286766559f;
const static float NX_4PI = 12.566370614359172953850573533118f;
const static float NX_INVPI = 0.31830988618379067153776752674503;

float Pow5(float x)
{
	float x2 = x * x;
	return x * x2 * x2;
}
