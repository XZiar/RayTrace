typedef struct clRay
{
	float3 ori, dir;
}clRay;

typedef struct clTri
{
	float3 axisu, axisv, p0;
}clTri;



__kernel void clTriTest(__constant clRay* ray, __global clTri* tri, __global float* res)
{
	int num = get_global_id(0);
	float3 tmp1 = cross(ray->dir, tri[num].axisv);
	float a = dot(tri[num].axisu, tmp1);
	if (a < 1e-5f && a > -1e-5f)
	{
		res[num * 4 + 3] = 1e20f;
		return;
	}
	float f = 1 / a;
	float3 t2r = ray->ori - tri[num].p0;
	float u = dot(t2r, tmp1) * f;
	if (u < 0.0f || u > 1.0f)
	{
		res[num * 4 + 3] = 1e20f;
		return;
	}
	float3 tmp2 = cross(t2r, tri[num].axisu);
	float v = dot(ray->dir, tmp2) * f;
	float duv = 1 - u - v;
	if (v < 0.0f || duv < 0.0f)
	{
		res[num * 4 + 3] = 1e20f;
		return;
	}
	float t = dot(tri[num].axisv, tmp2) * f;
	if (t > 1e-6f)
	{
		res[num * 4 + 3] = t;
		res[num * 4] = duv;
		res[num * 4 + 1] = u, res[num * 4 + 2] = v;
		return;
	}
	else
	{
		res[num * 4 + 3] = 1e20f;
		return;
	}
}
