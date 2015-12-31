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
	const double dp = tan(cam.fovy * PI / 360) / (height / 2);
	const float zNear = cam.zNear, zFar = cam.zFar;

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
	const float zNear = cam.zNear, zFar = cam.zFar;

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

Color RayTracer::RTdepth(const float zNear, const float zFar, const Ray &baseray)
{
	HitRes hr;
	for (auto dobj : scene->Objects)
	{
		if (dobj->bShow)
			hr = dobj->intersect(baseray, hr);
	}
	return Color(hr.distance, zNear, zFar);
}

Color RayTracer::RTnorm(const float zNear, const float zFar, const Ray &baseray)
{
	HitRes hr;
	for (auto dobj : scene->Objects)
	{
		if (dobj->bShow)
			hr = dobj->intersect(baseray, hr);
	}
	if (hr.distance > zFar)
		return Color(true);
	if (hr.distance < zNear)
		return Color(false);
	return Color(hr.normal);
}

Color RayTracer::RTtex(const float zNear, const float zFar, const Ray &baseray)
{
	HitRes hr;
	for (auto dobj : scene->Objects)
	{
		if (dobj->bShow)
			hr = dobj->intersect(baseray, hr);
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
		c.r = c.g = c.b = 150;
		return c;
	}
}

Color RayTracer::RTmtl(const float zNear, const float zFar, const Ray &baseray)
{
	HitRes hr;
	Color c(false);
	//c.r = c.g = c.b = 0.588 * 255;
	for (auto dobj : scene->Objects)
	{
		if (dobj->bShow)
			hr = dobj->intersect(baseray, hr);
	}
	if (hr.distance > zFar)
		return Color(true);
	if (hr.distance < zNear)
		return Color(false);
	if (hr.tex != nullptr)//has texture
		c = Color(hr.tex->w, hr.tex->h, hr.tex->data, hr.tcoord);
	Vertex vc_specular(255, 255, 255),
		vc(c.r, c.g, c.b),
		mix_vd, mix_va, mix_vsc;
	for (auto &lit : scene->Lights)
	{
		if (lit.bLight)
		{
			float light_lum;
			Normal p2l;
			//consider light type
			if (lit.position.alpha > 1e-6)
			{//point light
				Vertex p2l_v = lit.position - hr.position;
				float dis = p2l_v.length_sqr();
				float step = lit.attenuation.x
					+ lit.attenuation.y * sqrt(dis)
					+ lit.attenuation.z * dis;
				light_lum = 1 / step;
				p2l = Normal(p2l_v);
			}
			else
			{//parallel light
				light_lum = 1.0f;
				p2l = Normal(lit.position);
			}
			
			Vertex light_a = lit.ambient * light_lum,
				light_d = lit.diffuse * light_lum,
				light_s = lit.specular * light_lum;

			/*
			** ambient_color = base_map (*) mat_ambient (*) light_ambient
			*/
			Vertex v_ambient = hr.mtl->ambient.mixmul(light_a);
			mix_va += v_ambient;//ambient color
			/*
			** diffuse_color = base_map * normal.p2l (*) mat_diffuse (*) light_diffuse
			** p2l = normal that point towards light
			*/
			float n_n = hr.normal & p2l;
			if (n_n > 0)
			{
				Vertex v_diffuse = hr.mtl->diffuse.mixmul(light_d);
				mix_vd += v_diffuse * n_n;//diffuse color
			}
			/*
			** phong model
			** specular_color = (r2p'.p2l)^shiness * mat_diffuse (*) light_diffuse
			** p2l = normal that point towards light
			** r2p' = reflect normal that camera towards point
			** r2p' = r2p - 2 * (r2p.normal) * normal
			*/
			/*n_n = 2 * (baseray.direction & hr.normal);
			Normal r2p_r = baseray.direction - (hr.normal * n_n);
			n_n = r2p_r & p2l;
			if (n_n > 0)
			{
				Vertex v_specular = hr.mtl->specular.mixmul(light_s),
					vc_specular = Vertex(255, 255, 255);
				Vertex vs = v_specular * pow(n_n, log(hr.mtl->shiness.x) / log(1.28));
				mix_vsc += vc_specular.mixmul(vs);
			}*/
			/*
			** blinn-phong model
			** specular_color = (normal.h)^shiness * mat_diffuse (*) light_diffuse
			** h = Normalized(p2r + p2l)
			** p2r = normal that point towards camera = -r2p
			** p2l = normal that point towards light
			*/
			Normal h = Normal(p2l - baseray.direction);
			n_n = hr.normal & h;
			if (n_n > 0)
			{
				Vertex v_specular = hr.mtl->specular.mixmul(light_s);
				Vertex vs = v_specular * pow(n_n, hr.mtl->shiness.x);
				mix_vsc += vc_specular.mixmul(vs);
			}
		}
	}
	mix_va += hr.mtl->ambient.mixmul(scene->EnvLight);//environment ambient color
	return Color(vc.mixmul(mix_vd + mix_va) + mix_vsc);
}

Color RayTracer::RTshd(const float zNear, const float zFar, const Ray &baseray)
{
	HitRes hr;
	Color c(false);
	//c.r = c.g = c.b = 0.588 * 255;
	for (auto dobj : scene->Objects)
	{
		if (dobj->bShow)
			hr = dobj->intersect(baseray, hr);
	}
	if (hr.distance > zFar)
		return Color(true);
	if (hr.distance < zNear)
		return Color(false);
	if (hr.tex != nullptr)//has texture
		c = Color(hr.tex->w, hr.tex->h, hr.tex->data, hr.tcoord);
	Vertex vc_specular(255, 255, 255),
		vc(c.r, c.g, c.b),
		mix_vd, mix_va, mix_vsc;
	for (auto &lit : scene->Lights)
	{
		if (lit.bLight)
		{
			float light_lum;
			Normal p2l;
			float dis;
			//consider light type
			if (lit.position.alpha > 1e-6)
			{//point light
				Vertex p2l_v = lit.position - hr.position;
				dis = p2l_v.length_sqr();
				float step = lit.attenuation.x
					+ lit.attenuation.z * dis;
				dis = sqrt(dis);
				step += lit.attenuation.y * dis;
				light_lum = 1 / step;
				p2l = Normal(p2l_v);
			}
			else
			{//parallel light
				dis = 1e20;
				light_lum = 1.0f;
				p2l = Normal(lit.position);
			}

			Vertex light_a = lit.ambient * light_lum,
				light_d = lit.diffuse * light_lum,
				light_s = lit.specular * light_lum;

			/*
			** ambient_color = base_map (*) mat_ambient (*) light_ambient
			*/
			Vertex v_ambient = hr.mtl->ambient.mixmul(light_a);
			mix_va += v_ambient;//ambient color

			//shadow test
			Ray shadowray(hr.position, p2l);
			HitRes shr(dis);
			for (auto dobj : scene->Objects)
			{
				if (dobj->bShow)
					shr = dobj->intersect(shadowray, shr);
				if (shr.distance < dis)
					break;
			}
			if (shr.distance < dis)
				continue;
			/*
			** diffuse_color = base_map * normal.p2l (*) mat_diffuse (*) light_diffuse
			** p2l = normal that point towards light
			*/
			float n_n = hr.normal & p2l;
			if (n_n > 0)
			{
				Vertex v_diffuse = hr.mtl->diffuse.mixmul(light_d);
				mix_vd += v_diffuse * n_n;//diffuse color
			}
			/*
			** blinn-phong model
			** specular_color = (normal.h)^shiness * mat_diffuse (*) light_diffuse
			** h = Normalized(p2r + p2l)
			** p2r = normal that point towards camera = -r2p
			** p2l = normal that point towards light
			*/
			Normal h = Normal(p2l - baseray.direction);
			n_n = hr.normal & h;
			if (n_n > 0)
			{
				Vertex v_specular = hr.mtl->specular.mixmul(light_s);
				Vertex vs = v_specular * pow(n_n, hr.mtl->shiness.x);
				mix_vsc += vc_specular.mixmul(vs);
			}
		}
	}
	mix_va += hr.mtl->ambient.mixmul(scene->EnvLight);//environment ambient color
	return Color(vc.mixmul(mix_vd + mix_va) + mix_vsc);
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
	for (auto dobj : scene->Objects)
	{
		if (dobj->bShow)
			dobj->RTPrepare();
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
	case MY_MODEL_SHADOWTEST:
		fun = bind(&RayTracer::RTshd, this, _1, _2, _3);
		break;
	case MY_MODEL_RAYTRACE:
		fun = bind(&RayTracer::RTshd, this, _1, _2, _3);
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
