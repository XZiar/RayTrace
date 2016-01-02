typedef struct clRay
{
	float origin[4];
	float direction[4];
}clRay;

typedef struct clTri
{
	float points[12];
	float axisu[4], axisv[4];
	float norms[12];
	float tcoords[6];
}



__kernel void hello(__global clRay* ray, __global clTri* tri, __global float* res)
{
	Vertex tmp1 = ray.direction * tri.axisv;
	float a = tri.axisu & tmp1;
	if (abs(a) < 1e-6f)
		return 1e20f;
	float f = 1 / a;
	Vertex t2r = ray.origin - tri.points[0];
	float u = (t2r & tmp1) * f;
	if (u < 0.0f || u > 1.0f)
		return 1e20f;
	Vertex tmp2 = t2r * tri.axisu;
	float v = (ray.direction & tmp2) * f,
		duv = 1 - u - v;
	if (v < 0.0f || duv < 0.0f)
		return 1e20;
	float t = (tri.axisv & tmp2) * f;
	if (t > 1e-6f)
	{
		coord = Vertex(duv, u, v);
		return t;
	}
	else
		return 1e20;
}
