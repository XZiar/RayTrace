#include "rely.h"
#include "RayTracer.h"


void RayTracer::parallelRT(const int8_t tNum, const int8_t tID, const PR &worker)
{
	//calc time
	LARGE_INTEGER t_s, t_e, t_f;
	QueryPerformanceFrequency(&t_f);
	QueryPerformanceCounter(&t_s);

	const Camera &cam = scene->cam;
	const int32_t blk_h = height / 64, blk_w = width / 64;
	const float dp = tan(cam.fovy * PI / 360) / (height / 2);
	const double zNear = cam.zNear, zFar = cam.zFar;

	int16_t blk_cur = tID, blk_xcur = tID % blk_w, blk_ycur = tID / blk_w;
	while (blk_ycur < blk_h)
	{
		uint8_t *out_cur = output + blk_ycur * 64 * (3 * width) + blk_xcur * 64 * 3;
		for (auto ycur = blk_ycur * 64 - height / 2, ymax = ycur + 64; ycur < ymax; ++ycur)//pur y-line
		{
			for (auto xcur = blk_xcur * 64 - width / 2, xmax = xcur + 64; xcur < xmax; ++xcur)//per pixel
			{
				Vertex dir = cam.n + cam.u*(xcur*dp) + cam.v*(ycur*dp);
				Ray baseray(cam.position, dir);
				Color c = worker(zNear, zFar, baseray);
				if (!isRun)
				{
					state[tID] = true;
					return;
				}
				c.put(out_cur);
				out_cur += 3;
			}
			out_cur += (width - 64) * 3;
		}
		blk_cur = aBlock_Cur.fetch_add(1);
		blk_xcur = blk_cur % blk_w, blk_ycur = blk_cur / blk_w;
	}
	state[tID] = true;
	QueryPerformanceCounter(&t_e);
	costtime[tID] = (t_e.QuadPart - t_s.QuadPart)*1.0 / t_f.QuadPart;
}


void RayTracer::RTcheck(const int8_t tNum, const int8_t tID)
{
	const Camera &cam = scene->cam;
	const int32_t blk_h = height / 64, blk_w = width / 64;
	const float dp = tan(cam.fovy * PI / 360) / (height / 2);
	const double zNear = cam.zNear, zFar = cam.zFar;

	int16_t blk_cur = tID, blk_xcur = tID % blk_w, blk_ycur = tID / blk_w;
	while (blk_ycur < blk_h)
	{
		Color c((blk_ycur & 0x1) == (blk_xcur & 0x1));
		uint8_t *out_cur = output + blk_ycur * 64 * (3 * width) + blk_xcur * 64 * 3;
		for (auto ycur = blk_ycur * 64 - height / 2, ymax = ycur + 64; ycur < ymax; ++ycur)//pur y-line
		{
			for (auto xcur = blk_xcur * 64 - width / 2, xmax = xcur + 64; xcur < xmax; ++xcur)//per pixel
			{
				if (!isRun)
				{
					state[tID] = true;
					return;
				}
				c.put(out_cur);
				out_cur += 3;
			}
			out_cur += (width - 64) * 3;
			Sleep(1);
		}
		blk_cur = aBlock_Cur.fetch_add(1);
		blk_xcur = blk_cur % blk_w, blk_ycur = blk_cur / blk_w;
	}
	state[tID] = true;
}

Color RayTracer::RTdepth(const double zNear, const double zFar, const Ray &baseray)
{
	HitRes hr;
	for (auto t : scene->Objects)
	{
		if (get<1>(t))
			hr = get<0>(t)->intersect(baseray, hr);
	}
	return Color(hr.distance, zNear, zFar);
}

Color RayTracer::RTnorm(const double zNear, const double zFar, const Ray &baseray)
{
	HitRes hr;
	for (auto t : scene->Objects)
	{
		if (get<1>(t))
			hr = get<0>(t)->intersect(baseray, hr);
	}
	if (hr.distance > zFar)
		return Color(true);
	if (hr.distance < zNear)
		return Color(false);
	return Color(hr.normal);
}

Color RayTracer::RTtex(const double zNear, const double zFar, const Ray &baseray)
{
	HitRes hr;
	for (auto t : scene->Objects)
	{
		if (get<1>(t))
			hr = get<0>(t)->intersect(baseray, hr);
	}
	if (hr.distance > zFar)
		return Color(true);
	if (hr.distance < zNear)
		return Color(false);
	if (hr.tex != nullptr)//has texture
	{
		return Color(hr.tex->w, hr.tex->h, hr.tex->data, hr.tcoord);
	}
	else
	{
		Color c(false);
		c.r = c.g = c.b = 0.588 * 255;
		return c;
	}
}

Color RayTracer::RTmtl(const double zNear, const double zFar, const Ray &baseray)
{
	HitRes hr;
	Color c(false);
	c.r = c.g = c.b = 0.588 * 255;
	for (auto t : scene->Objects)
	{
		if (get<1>(t))
			hr = get<0>(t)->intersect(baseray, hr);
	}
	if (hr.distance > zFar)
		return Color(true);
	if (hr.distance < zNear)
		return Color(false);
	if (hr.tex != nullptr)//has texture
		c = Color(hr.tex->w, hr.tex->h, hr.tex->data, hr.tcoord);
	Vertex vc(c.r, c.g, c.b), mix_vdc, mix_vac, mix_vsc;
	for (auto a = 0; a < 8; ++a)
	{
		Light &lit = scene->Lights[a];
		if (lit.bLight)
		{
			/*
			** ambient_color = base_map (*) mat_ambient (*) light_ambient
			*/
			Vertex v_ambient = hr.mtl->ambient.mixmul(lit.ambient);
			mix_vac += v_ambient;//ambient color
			/*
			** diffuse_color = base_map * normal.p2l (*) mat_diffuse (*) light_diffuse
			** p2l = normal that point towards light
			*/
			Normal p2l = Normal(lit.position.alpha>1e-6 ? lit.position - hr.position : lit.position);
			float n_n = hr.normal & p2l;
			if (n_n > 0)
			{
				Vertex v_diffuse = hr.mtl->diffuse.mixmul(lit.diffuse);
				mix_vdc += v_diffuse * n_n;//diffuse color
			}
			/*
			** phong model//blinn-phong model
			** specular_color = (r2p'.p2l)^shiness * mat_diffuse (*) light_diffuse
			** p2l = normal that point towards light
			** r2p' = reflect normal that camera towards point
			** r2p' = r2p - 2 * (r2p.normal) * normal
			*/
			n_n = 2 * (baseray.direction & hr.normal);
			Normal r2p_r = baseray.direction - (hr.normal * n_n);
			n_n = r2p_r & p2l;
			if (n_n > 0)
			{
				Vertex v_specular = hr.mtl->specular.mixmul(lit.specular);
				mix_vsc += v_specular * pow(n_n, hr.mtl->shiness.x);
			}
		}
	}
	return Color(vc.mixmul(mix_vdc + mix_vac + mix_vsc));
}




RayTracer::RayTracer(Scene &scene)
{
	this->scene = &scene;
	output = new uint8_t[2048 * 2048 * 3];
	memset(output, 127, 2048 * 2048 * 3);
	glGenTextures(1, &texID);
}


RayTracer::~RayTracer()
{
	delete[] output;
}

void RayTracer::start(const uint8_t type, const int8_t tnum)
{
	isFinish = false;
	isRun = true;
	width = scene->cam.width;
	height = scene->cam.height;
	memset(output, 127, 2048 * 2048 * 3);
	for (auto t : scene->Objects)
	{
		if (get<1>(t))
			get<0>(t)->RTPrepare();
	}
	aBlock_Cur = tnum;
	PR fun;
	switch (type)
	{
	case MY_MODEL_DEPTHTEST:
		fun = bind(&RayTracer::RTdepth, this, _1, _2, _3);
		break;
	case MY_MODEL_NORMALTEST:
		fun = bind(&RayTracer::RTnorm, this, _1, _2, _3);
		break;
	case MY_MODEL_TEXTURETEST:
		fun = bind(&RayTracer::RTtex, this, _1, _2, _3);
		break;
	case MY_MODEL_MATERIALTEST:
		fun = bind(&RayTracer::RTmtl, this, _1, _2, _3);
		break;

	case MY_MODEL_RAYTRACE:
		fun = bind(&RayTracer::RTtex, this, _1, _2, _3);
		break;
	}
	for (int8_t a = 0; a < tnum; a++)
	{
		state[a] = false;
		costtime[a] = 0.0;
		if (type == MY_MODEL_CHECK)
			t[a] = thread(mem_fn(&RayTracer::RTcheck), this, tnum, a);
		else
			t[a] = thread(mem_fn(&RayTracer::parallelRT), this, tnum, a, fun);
		t[a].detach();
	}
	auto tmain = thread([&, tnum]
	{
		double time;
		while (true)
		{
			bool isOK = true;
			time = 0.0;
			for (int8_t a = 0;isOK && a < tnum; a++)
			{
				time += costtime[a];
				if (state[a] == false)
					isOK = false;
			}
			if (isOK)
				break;
			Sleep(50);
		}
		isFinish = true;
		isRun = false;
		useTime = time / tnum;
	});
	tmain.detach();
}

void RayTracer::stop()
{
	isRun = false;
}
