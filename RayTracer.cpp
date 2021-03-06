#include "rely.h"
#include "RayTracer.h"


void RayTracer::parallelRT(const int8_t tNum, const int8_t tID, const PerRay &worker)
{
	//calc time
	LARGE_INTEGER t_s, t_e, t_f;
	QueryPerformanceFrequency(&t_f);
	QueryPerformanceCounter(&t_s);

	const Camera &cam = scene->cam;
	const int32_t blk_h = height / 64, blk_w = width / 64;
	const double dp = tan(cam.fovy * PI / 360) / (height / 2);
	const float zNear = cam.zNear, zFar = sqrt(2)*cam.zFar;

	int16_t blk_cur = tID, blk_xcur = tID % blk_w, blk_ycur = tID / blk_w;
	while (blk_ycur < blk_h)
	{
		uint8_t *out_cur = output + blk_ycur * 64 * (3 * width) + blk_xcur * 64 * 3;
		for (auto ycur = blk_ycur * 64 - height / 2, ymax = ycur + 64; ycur < ymax; ++ycur)//pur y-line
		{
			
			for (auto xcur = blk_xcur * 64 - width / 2, xmax = xcur + 64; xcur < xmax; ++xcur)//per pixel
			{
				Vertex dir = cam.n + cam.u*(xcur*dp) + cam.v*(ycur*dp);
				Ray baseray(cam.position, dir, MY_RAY_BASERAY);
				Color c = (this->*worker)(zNear, zFar, baseray);
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
	Color c(true);
	c.set(hr.distance, zNear, zFar);
	return c;
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
		return Color(false);
	if (hr.distance < zNear)
		return Color(true);
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
		return Color(false);
	if (hr.distance < zNear)
		return Color(true);
	if (hr.tex != nullptr)//has texture
		return Color(hr.tex, hr.tcoord);
	else
		return Color(0.588f, 0.588f, 0.588f);
}

Color RayTracer::RTmtl(const float zNear, const float zFar, const Ray &baseray)
{
	HitRes hr;
	for (auto dobj : scene->Objects)
	{
		if (dobj->bShow)
			hr = dobj->intersect(baseray, hr);
	}
	if (hr.distance > zFar)
		return Color(false);
	if (hr.distance < zNear)
		return Color(true);
	Color vc_specular(1.0f, 1.0f, 1.0f),
		vc(hr.tex, hr.tcoord),
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
				Vertex vs = v_specular * pow(n_n, hr.mtl->shiness);
				mix_vsc += vc_specular.mixmul(vs);
			}
		}
	}
	mix_va += hr.mtl->ambient.mixmul(scene->EnvLight);//environment ambient color
	return vc.mixmul(mix_vd + mix_va) + mix_vsc;
}

Color RayTracer::RTshd(const float zNear, const float zFar, const Ray &baseray)
{
	HitRes hr;
	for (auto dobj : scene->Objects)
	{
		if (dobj->bShow)
			hr = dobj->intersect(baseray, hr);
	}
	if (hr.distance > zFar)
		return Color(false);
	if (hr.distance < zNear)
		return Color(true);
	Color vc_specular(1.0f, 1.0f, 1.0f),
		vc(hr.tex, hr.tcoord),
		mix_vd, mix_va, mix_vsc;
	for (auto &lit : scene->Lights)
	{
		if (lit.bLight)
		{
			float light_lum;
			Normal p2l;
			float dis;
			//consider light type
			if (lit.type == MY_LIGHT_POINT)
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
				dis = 1e10;
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
			shr.obj = hr.obj;
			for (auto dobj : scene->Objects)
			{
				if (dobj->bShow)
					//quick test to find nearest blocking object
					shr = dobj->intersect(shadowray, shr, dis);
				//early quick
				if (shr.distance < dis)
					break;
			}
			if (shr.distance < dis)//something block the light
			{
				//mix_vd += Vertex(1, 0, 0);
				continue;
			}
				
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
				Vertex vs = v_specular * pow(n_n, hr.mtl->shiness);
				mix_vsc += vc_specular.mixmul(vs);
			}
		}
	}
	mix_va += hr.mtl->ambient.mixmul(scene->EnvLight);//environment ambient color
	return vc.mixmul(mix_vd + mix_va) + mix_vsc;
}

Color RayTracer::proxyRTflec(const float zNear, const float zFar, const Ray & baseray)
{
	return RTflec(zNear, zFar, baseray, 0, 1.0f, tmphr);
}
Color RayTracer::proxyRTfrac(const float zNear, const float zFar, const Ray & baseray)
{
	return RTfrac(zNear, zFar, baseray, 0, 1.0f, tmphr);
}

Color RayTracer::RTflec(const float zNear, const float zFar, const Ray & baseray,
	const uint8_t level, const float bwc, HitRes & basehr)
{
	if (level > maxLevel || bwc < 1e-5f)//deep limit
		return Color(false);
	HitRes hr = basehr;
	intptr_t newobj = basehr.obj;
	//intersect
	for (auto dobj : scene->Objects)
		if (dobj->bShow)
		{
			hr.obj = basehr.obj;
			hr = dobj->intersect(baseray, hr);
			if (hr.obj != basehr.obj)
				newobj = hr.obj;
		}
	//early cut
	if (hr.distance > zFar || hr.distance < zNear)
		return Color(false);
	Color vc_specular(1.0f, 1.0f, 1.0f),
		vc(hr.tex, hr.tcoord),
		mix_vd, mix_vsc;
	Vertex mix_va = hr.mtl->ambient.mixmul(scene->EnvLight);//environment ambient color
	//accept light
	for (auto &lit : scene->Lights)
	{
		if (lit.bLight)
		{
			Vertex light_a, light_d, light_s;
			Normal p2l;
			float dis;
			//consider light type
			if (lit.type == MY_LIGHT_POINT)
			{//point light
				Vertex p2l_v = lit.position - hr.position;
				dis = p2l_v.length_sqr();
				float step = lit.attenuation.x
					+ lit.attenuation.z * dis;
				dis = sqrt(dis);
				step += lit.attenuation.y * dis;
				float light_lum = 1 / step;
				light_a = lit.ambient * light_lum;
				light_d = lit.diffuse * light_lum;
				light_s = lit.specular * light_lum;
				p2l = Normal(p2l_v);
			}
			else
			{//parallel light
				dis = 1e10;
				light_a = lit.ambient;
				light_d = lit.diffuse;
				light_s = lit.specular;
				p2l = Normal(lit.position);
			}
			/*
			** ambient_color = base_map (*) mat_ambient (*) light_ambient
			*/
			Vertex v_ambient = hr.mtl->ambient.mixmul(light_a);
			mix_va += v_ambient;
			//shadow test
			Ray shadowray(hr.position, p2l);
			HitRes shr(dis);
			shr.obj = newobj;
			for (auto dobj : scene->Objects)
			{
				if (dobj->bShow)
					shr = dobj->intersect(shadowray, shr, dis);//quick test to find not the nearest blocking object
				//early quit
				if (shr.distance < dis)//something block the light
					goto ____EOLT;
			}
			/*
			** diffuse_color = base_map * normal.p2l (*) mat_diffuse (*) light_diffuse
			*/
			float n_n = hr.normal & p2l;
			if (n_n > 0)
			{
				Vertex v_diffuse = hr.mtl->diffuse.mixmul(light_d);
				mix_vd += v_diffuse * n_n;
			}
			/*
			** blinn-phong model
			** specular_color = (normal.h)^shiness * mat_diffuse (*) light_diffuse
			** h = Normalized(p2r + p2l)
			*/
			Normal h = Normal(p2l - baseray.direction);
			n_n = hr.normal & h;
			if (n_n > 0)
			{
				Vertex v_specular = hr.mtl->specular.mixmul(light_s);
				Vertex vs = v_specular * pow(n_n, hr.mtl->shiness);
				mix_vsc += vc_specular.mixmul(vs);
			}
		}
	____EOLT:;//end of light test this turn
	}
	Color c_all = vc.mixmul(mix_vd + mix_va) + mix_vsc;
	//accept reflection
	if (hr.mtl->reflect > 0.01f)
	{
		const float &flecrate = hr.mtl->reflect;
		//reflection test
		c_all *= (1 - flecrate);
		/*
		** r2p' = reflect normal that camera towards point
		** r2p' = r2p - 2 * (r2p.normal) * normal
		*/
		float n_n = 2 * (baseray.direction & hr.normal);
		Normal r2p_r = baseray.direction - (hr.normal * n_n);
		Ray flecray(hr.position, r2p_r);
		HitRes flechr;
		flechr.obj = newobj;
		Color c_flec = RTflec(0.0f, zFar, flecray, level + 1, bwc * flecrate, flechr);
		c_all += c_flec * flecrate;
	}
	return c_all ;
}

Color RayTracer::RTfrac(const float zNear, const float zFar, const Ray & baseray,
	const uint8_t level, const float bwc, HitRes & basehr)
{
	if (level > maxLevel || bwc < 1e-5f)//deep limit
		return Color(false);
	HitRes hr = basehr;
	intptr_t newobj = basehr.obj;
	//intersect
	for (auto dobj : scene->Objects)
		if (dobj->bShow)
		{
			hr.obj = basehr.obj;
			hr = dobj->intersect(baseray, hr);
			if (hr.obj != basehr.obj)
				newobj = hr.obj;
		}
	//early cut
	if (hr.distance > zFar || hr.distance < zNear)
		return Color(false);
	Color vc_specular(1.0f, 1.0f, 1.0f),
		vc(hr.tex, hr.tcoord),
		mix_vd, mix_vsc;
	Vertex mix_va = hr.mtl->ambient.mixmul(scene->EnvLight);//environment ambient color
															//accept light
	for (auto &lit : scene->Lights)
	{
		if (lit.bLight)
		{
			Vertex light_a, light_d, light_s;
			Normal p2l;
			float dis;
			//consider light type
			if (lit.type == MY_LIGHT_POINT)
			{//point light
				Vertex p2l_v = lit.position - hr.position;
				dis = p2l_v.length_sqr();
				float step = lit.attenuation.x
					+ lit.attenuation.z * dis;
				dis = sqrt(dis);
				step += lit.attenuation.y * dis;
				float light_lum = 1 / step;
				light_a = lit.ambient * light_lum;
				light_d = lit.diffuse * light_lum;
				light_s = lit.specular * light_lum;
				p2l = Normal(p2l_v);
			}
			else
			{//parallel light
				dis = 1e10;
				light_a = lit.ambient;
				light_d = lit.diffuse;
				light_s = lit.specular;
				p2l = Normal(lit.position);
			}
			/*
			** ambient_color = base_map (*) mat_ambient (*) light_ambient
			*/
			Vertex v_ambient = hr.mtl->ambient.mixmul(light_a);
			mix_va += v_ambient;
			//shadow test
			Ray shadowray(hr.position, p2l, MY_RAY_SHADOWRAY);
			HitRes shr(dis);
			shr.obj = newobj;
			for (auto dobj : scene->Objects)
			{
				if (dobj->bShow)
					shr = dobj->intersect(shadowray, shr, dis);//quick test to find not the nearest blocking object
															   //early quit
				if (shr.distance < dis)//something block the light
					goto ____EOLT;
			}
			/*
			** diffuse_color = base_map * normal.p2l (*) mat_diffuse (*) light_diffuse
			*/
			float n_n = hr.normal & p2l;
			if (n_n > 0)
			{
				Vertex v_diffuse = hr.mtl->diffuse.mixmul(light_d);
				mix_vd += v_diffuse * n_n;
			}
			/*
			** blinn-phong model
			** specular_color = (normal.h)^shiness * mat_diffuse (*) light_diffuse
			** h = Normalized(p2r + p2l)
			*/
			Normal h = Normal(p2l - baseray.direction);
			n_n = hr.normal & h;
			if (n_n > 0)
			{
				Vertex v_specular = hr.mtl->specular.mixmul(light_s);
				Vertex vs = v_specular * pow(n_n, hr.mtl->shiness);
				mix_vsc += vc_specular.mixmul(vs);
			}
		}
	____EOLT:;//end of light test this turn
	}
	Color c_all = vc.mixmul(mix_vd + mix_va) + mix_vsc;
	//accept reflection
	if (hr.mtl->reflect > 0.01f)
	{
		const float &flecrate = hr.mtl->reflect;
		//reflection test
		c_all *= (1 - flecrate);
		/*
		** r2p' = reflect normal that camera towards point
		** r2p' = r2p - 2 * (r2p.normal) * normal
		*/
		float n_n = 2 * (baseray.direction & hr.normal);
		Normal r2p_r = baseray.direction - (hr.normal * n_n);
		Ray flecray(hr.position, r2p_r, MY_RAY_REFLECTRAY);
		HitRes flechr;
		flechr.obj = newobj;
		Color c_flec = RTfrac(0.0f, zFar, flecray, level + 1, bwc * flecrate, flechr);
		c_all += c_flec * flecrate;
	}
	if (hr.mtl->refract > 0.01f)
	{
		const float &fracrate = hr.mtl->refract;
		//refraction test
		c_all *= (1 - fracrate);

		float n = baseray.mtlrfr / hr.rfr;
		float cosIn = -(baseray.direction & hr.normal);
		float cosOut2 = 1.0f - (n * n) * (1.0f - cosIn * cosIn);
		if (cosOut2 < 0.0f)//ȫ����
			goto ____EOFR;
		Vertex l2 = baseray.direction * n,
			l1 = hr.normal * (n*cosIn - sqrt(cosOut2));

		Ray fracray(hr.position, l1 + l2, MY_RAY_REFRACTRAY);
		fracray.mtlrfr = hr.rfr;
		fracray.isInside = hr.isInside;
		HitRes frachr;
		frachr.obj = newobj;
		Color c_frac = RTfrac(0.0f, zFar, fracray, level + 1, bwc * fracrate, frachr);
		Color vc_frac(1, 1, 1);
		if (hr.isInside)//peer's law, not sure if useful
		{
			vc_frac = hr.mtl->diffuse * 0.15f * -c_frac.alpha;
			vc_frac = Vertex(exp(vc_frac.r), exp(vc_frac.g), exp(vc_frac.b));
		}
		c_all += c_frac.mixmul(vc_frac) * fracrate;
	}
____EOFR:;//end of fraction test
	c_all.alpha = hr.distance;
	return c_all;
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

	PerRay fun;
	switch (type)
	{
	case MY_MODEL_DEPTHTEST:
		fun = &RayTracer::RTdepth;
		//fun = bind(&RayTracer::RTdepth, this, _1, _2, _3);
		break;
	case MY_MODEL_NORMALTEST:
		fun = &RayTracer::RTnorm;
		//fun = bind(&RayTracer::RTnorm, this, _1, _2, _3);
		break;
	case MY_MODEL_TEXTURETEST:
		fun = &RayTracer::RTtex;
		//fun = bind(&RayTracer::RTtex, this, _1, _2, _3);
		break;
	case MY_MODEL_MATERIALTEST:
		fun = &RayTracer::RTmtl;
		//fun = bind(&RayTracer::RTmtl, this, _1, _2, _3);
		break;
	case MY_MODEL_SHADOWTEST:
		fun = &RayTracer::RTshd;
		//fun = bind(&RayTracer::RTshd, this, _1, _2, _3);
		break;
	case MY_MODEL_REFLECTTEST:
		fun = &RayTracer::proxyRTflec;
		//fun = bind(&RayTracer::RTflec, this, _1, _2, _3, 0, 1.0f, tmphr);
		break;
	case MY_MODEL_REFRACTTEST:
		fun = &RayTracer::proxyRTfrac;
		//fun = bind(&RayTracer::RTfrac, this, _1, _2, _3, 0, 1.0f, tmphr);
		break;
	case MY_MODEL_RAYTRACE:
		fun = &RayTracer::proxyRTfrac;
		//fun = bind(&RayTracer::RTfrac, this, _1, _2, _3, 0, 1.0f, tmphr);
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
