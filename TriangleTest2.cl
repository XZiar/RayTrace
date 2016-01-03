#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

typedef struct clRay
{
	float3 ori, dir;
}clRay;

typedef struct clTri
{
	float3 axisu, axisv, p0;
}clTri;



__kernel void clTriTest(__global const clRay* ray, __global clTri* tri, __global float4* finans)
{
	__local float4 tres[1024];
	__local volatile unsigned int passcnt;
	int num = get_global_id(0);
	if (num == 0)
	{
		passcnt = 0;
		tres[0] = (float4)(0.0f, 0.0f, 0.0f, 1e20f);
	}
//	barrier(CLK_LOCAL_MEM_FENCE);

	float3 tmp1 = cross(ray->dir, tri[num].axisv);
	float a = dot(tri[num].axisu, tmp1);
	//printf("%f:%f\n", ray->dir.x, a);
	if (fabs(a) > 1e-5f)
	{
		float f = 1 / a;
		float3 t2r = ray->ori - tri[num].p0;
		float u = dot(t2r, tmp1) * f;
		//printf("\tstep1,f:%f,u:%f\n", f, u);
		if (u >= 0.0f && u <= 1.0f)
		{
			float3 tmp2 = cross(t2r, tri[num].axisu);
			float v = dot(ray->dir, tmp2) * f;
			float duv = 1 - u - v;
			//printf("\tstep2,v:%f,duv:%f\n", v, duv);
			if (v >= 0.0f && duv >= 0.0f)
			{
				float t = dot(tri[num].axisv, tmp2) * f;
				//printf("\tstep3,t:%f\n", t);
				if (t > 1e-6f)
				{
					//maybe the lowest
					unsigned int mypc = atomic_inc(&passcnt);
					tres[mypc].w = t;
					tres[mypc].x = as_float(num);
					tres[mypc].y = u, tres[mypc].z = v;
					//printf("%f\tpass\n",t);
					///return;
				}
				//printf("\ttfail\n");
			}
			//printf("\tuvfail\n");
		}
		//printf("\tufail\n");
	}
	//printf("unplane\n");
	//res[num * 4 + 3] = 1e20f;
	//return;
	barrier(CLK_LOCAL_MEM_FENCE);
	if(num == 0)
	{//find min
		while (--passcnt > 0)
			if (tres[passcnt].w < tres[0].w)
				tres[0] = tres[passcnt];
	}
	*finans = tres[0];
}
