#ifndef _SHIRRADIANCE_COMMON_
#define _SHIRRADIANCE_COMMON_

const static float g_SHFactor[] =
{
	0.28209479177387814347403972578039,
	-0.48860251190291992158638462283835,
	0.48860251190291992158638462283835,
	-0.48860251190291992158638462283835,
	1.0925484305920790705433857058027,
	-1.0925484305920790705433857058027,
	0.31539156525252000603089369029571,
	-1.0925484305920790705433857058027,
	0.54627421529603953527169285290135
};

float3 GetIrradiance(float3 v, float4 m_irradSH0123x, float4 m_irradSH4567x, float4 m_irradSH0123y, float4 m_irradSH4567y, float4 m_irradSH0123z, float4 m_irradSH4567z, float3 m_irradSH8xyz)
{
	float3 intensity;
	intensity.x =
		g_SHFactor[0] * m_irradSH0123x.x +
		g_SHFactor[1] * m_irradSH0123x.y * v.x +
		g_SHFactor[2] * m_irradSH0123x.z * v.y +
		g_SHFactor[3] * m_irradSH0123x.w * v.z +
		g_SHFactor[4] * m_irradSH4567x.x * v.x * v.z +
		g_SHFactor[5] * m_irradSH4567x.y * v.x * v.y +
		g_SHFactor[6] * m_irradSH4567x.z * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
		g_SHFactor[7] * m_irradSH4567x.w * v.y * v.z +
		g_SHFactor[8] * m_irradSH8xyz.x * (v.z * v.z - v.x * v.x);

	intensity.y =
		g_SHFactor[0] * m_irradSH0123y.x +
		g_SHFactor[1] * m_irradSH0123y.y * v.x +
		g_SHFactor[2] * m_irradSH0123y.z * v.y +
		g_SHFactor[3] * m_irradSH0123y.w * v.z +
		g_SHFactor[4] * m_irradSH4567y.x * v.x * v.z +
		g_SHFactor[5] * m_irradSH4567y.y * v.x * v.y +
		g_SHFactor[6] * m_irradSH4567y.z * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
		g_SHFactor[7] * m_irradSH4567y.w * v.y * v.z +
		g_SHFactor[8] * m_irradSH8xyz.y * (v.z * v.z - v.x * v.x);

	intensity.z =
		g_SHFactor[0] * m_irradSH0123z.x +
		g_SHFactor[1] * m_irradSH0123z.y * v.x +
		g_SHFactor[2] * m_irradSH0123z.z * v.y +
		g_SHFactor[3] * m_irradSH0123z.w * v.z +
		g_SHFactor[4] * m_irradSH4567z.x * v.x * v.z +
		g_SHFactor[5] * m_irradSH4567z.y * v.x * v.y +
		g_SHFactor[6] * m_irradSH4567z.z * (2.0 * v.y * v.y - v.z * v.z - v.x * v.x) +
		g_SHFactor[7] * m_irradSH4567z.w * v.y * v.z +
		g_SHFactor[8] * m_irradSH8xyz.z * (v.z * v.z - v.x * v.x);

	return intensity;
}

#endif // !_SHIRRADIANCE_COMMON_