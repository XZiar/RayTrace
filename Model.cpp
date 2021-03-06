#include "rely.h"
#include "Basic3DObject.h"
#include "Model.h"

#define pS(a,b,c) (a)*65536+(b)*256+(c)

static inline int32_t parseStr(string str)
{
	if (str.length() < 3)
		str += "****";
	return str[2] + 256 * str[1] + 65536 * str[0];
}

int32_t Model::loadobj(const wstring &objname, const uint8_t code)
{
	Loader ldr(objname);
	string ele[5];
	int8_t num, pn;
	int ti[16];

	bool bFirstT = true,
		bFirstO = true;

	//clean
	
	vers.reserve(8000);
	nors.reserve(8000);
	txcs.reserve(8000);
	//load
	vector<Triangle> tris;
	tris.reserve(2000);
	Vertex v;
	Normal n;
	Coord2D tc;
	Triangle t;
	vers.push_back(v);
	nors.push_back(n);
	txcs.push_back(tc);

	VerMin = Vertex(1000, 1000, 1000);
	VerMax = Vertex(-1000, -1000, -1000);

	auto Border = [](Vertex &Min, Vertex &Max, const Vertex &v)
	{
	#ifdef SSE2
		Min.dat = _mm_min_ps(Min.dat, v.dat);
		Max.dat = _mm_max_ps(Max.dat, v.dat);
	#else
		if (v.x > Max.x)
			Max.x = v.x;
		if (v.x < Min.x)
			Min.x = v.x;

		if (v.y > Max.y)
			Max.y = v.y;
		if (v.y < Min.y)
			Min.y = v.y;

		if (v.z > Max.z)
			Max.z = v.z;
		if (v.z < Min.z)
			Min.z = v.z;
	#endif
	};

	while (true)
	{
		num = ldr.read(ele);
		if (num == INT8_MIN)
			break;
		if (num == -1)
			continue;
		switch (parseStr(ele[0]))
		{
		case pS('v', '*', '*')://vertex
			v = Vertex(atof(ele[1].c_str()), atof(ele[2].c_str()), atof(ele[3].c_str()));
			vers.push_back(v);
			break;
		case pS('v', 'n', '*')://normal
			n = Normal(atof(ele[1].c_str()), atof(ele[2].c_str()), atof(ele[3].c_str()));
			nors.push_back(n);
			break;
		case pS('v', 't', '*')://texture coord
			tc = Coord2D(atof(ele[1].c_str()), atof(ele[2].c_str()));
			txcs.push_back(tc);
			break;
		case pS('f', '*', '*')://triangle
			if (num > 4)
			{
				pn = ldr.parseInt(ele[1], &ti[0]) + ldr.parseInt(ele[2], &ti[3]) + ldr.parseInt(ele[3], &ti[6]) + ldr.parseInt(ele[4], &ti[9]);
				if (pn < 12)
				{
					t = Triangle(vers[ti[0]], nors[ti[2]],
						vers[ti[3]], nors[ti[5]],
						vers[ti[6]], nors[ti[8]]);
					tris.push_back(t);
					t = Triangle(vers[ti[0]], nors[ti[2]],
						vers[ti[6]], nors[ti[8]],
						vers[ti[9]], nors[ti[11]]);
					tris.push_back(t);
				}
				else
				{
					t = Triangle(vers[ti[0]], nors[ti[2]], txcs[ti[1]],
						vers[ti[3]], nors[ti[5]], txcs[ti[4]],
						vers[ti[6]], nors[ti[8]], txcs[ti[7]]);
					tris.push_back(t);
					t = Triangle(vers[ti[0]], nors[ti[2]], txcs[ti[1]],
						vers[ti[6]], nors[ti[8]], txcs[ti[7]],
						vers[ti[9]], nors[ti[11]], txcs[ti[10]]);
					tris.push_back(t);
				}
				Border(VerMin, VerMax, vers[ti[0]]);
				Border(VerMin, VerMax, vers[ti[3]]);
				Border(VerMin, VerMax, vers[ti[6]]);
				Border(VerMin, VerMax, vers[ti[9]]);
			}
			else
			{
				pn = ldr.parseInt(ele[1], &ti[0]) + ldr.parseInt(ele[2], &ti[3]) + ldr.parseInt(ele[3], &ti[6]);
				if (pn < 9)
					t = Triangle(vers[ti[0]], nors[ti[2]],
						vers[ti[3]], nors[ti[5]],
						vers[ti[6]], nors[ti[8]]);
				else
					t = Triangle(vers[ti[0]], nors[ti[2]], txcs[ti[1]],
						vers[ti[3]], nors[ti[5]], txcs[ti[4]],
						vers[ti[6]], nors[ti[8]], txcs[ti[7]]);
				tris.push_back(t);
			#ifdef SSE2
				__m128 i1dat = _mm_min_ps(vers[ti[0]].dat, vers[ti[3]].dat);
				__m128 a1dat = _mm_max_ps(vers[ti[0]].dat, vers[ti[3]].dat);
				__m128 i2dat = _mm_min_ps(VerMin.dat, vers[ti[6]].dat);
				__m128 a2dat = _mm_max_ps(VerMax.dat, vers[ti[6]].dat);
				VerMin.dat = _mm_min_ps(i1dat, i2dat);
				VerMax.dat = _mm_max_ps(a1dat, a2dat);
			#else
				Border(VerMin, VerMax, vers[ti[0]]);
				Border(VerMin, VerMax, vers[ti[3]]);
				Border(VerMin, VerMax, vers[ti[6]]);
			#endif
			}
			break;
		case pS('u', 's', 'e')://object part
			if (!bFirstO)
			{
				tris.shrink_to_fit();
				parts.push_back(move(tris));
				tris.reserve(2000);
				borders.push_back(VerMin);
				borders.push_back(VerMax);
			}
			VerMin = Vertex(1000, 1000, 1000);
			VerMax = Vertex(-1000, -1000, -1000);
			bFirstO = false;
			int8_t a = mtls.size();
			while (--a > 0)
				if (mtls[a].name == ele[1])
					break;
			part_mtl.push_back(a);
			break;
		}
	}
	if (bFirstO)
	{
		part_mtl.push_back(0);
	}
	tris.shrink_to_fit();
	parts.push_back(move(tris));

	borders.push_back(VerMin);
	borders.push_back(VerMax);

	//Main Border
	{
		VerMin = Vertex(1000, 1000, 1000);
		VerMax = Vertex(-1000, -1000, -1000);

		for (auto a = 0; a < borders.size(); a += 2)
		{
			Border(VerMin, VerMax, borders[a]);
			Border(VerMin, VerMax, borders[a + 1]);
		}

		Vertex Dif = VerMax - VerMin;
		float mdif = Dif.x > Dif.y ? Dif.x : Dif.y;
		mdif = Dif.z > mdif ? Dif.z / 8.0 : mdif / 8.0;
		VerMax /= mdif, VerMin /= mdif;
		//resize borders
		for (Vertex &v : borders)
			v /= mdif;
		//resize triangles
		for (vector<Triangle> &vt : parts)
			for (Triangle &t : vt)
				for (Vertex &v : t.points)
					v /= mdif;
		//finish resize
	}
	
	return parts.size();
}

int32_t Model::loadmtl(const wstring &mtlname, const uint8_t code)
{
	Loader ldr(mtlname);
	string ele[5];
	int8_t num;
	int8_t cur_tex_id = -1;

	bool bFirstT = true,
		bFirstM = true;

	//clean

	Material mtl;
	mtls.push_back(mtl);
	mtl_tex.push_back(cur_tex_id);
	//load

	while (true)
	{
		num = ldr.read(ele);
		if (num == INT8_MIN)
			break;
		if (num == -1)
			continue;
		switch (parseStr(ele[0]))
		{
		case pS('n', 'e', 'w')://mtl part
			if (!bFirstM)
			{
				mtls.push_back(mtl);
				mtl_tex.push_back(cur_tex_id);
				//mtl = Material();
			}
			mtl.name = ele[1];
			cur_tex_id = -1;
			bFirstM = false;
			break;
		case pS('K', 'a', '*')://ambient
			mtl.SetMtl(MY_MODEL_AMBIENT, atof(ele[1].c_str()), atof(ele[2].c_str()), atof(ele[3].c_str()));
			break;
		case pS('K', 'd', '*')://diffuse
			mtl.SetMtl(MY_MODEL_DIFFUSE, atof(ele[1].c_str()), atof(ele[2].c_str()), atof(ele[3].c_str()));
			break;
		case pS('K', 's', '*')://specular
			mtl.SetMtl(MY_MODEL_SPECULAR, atof(ele[1].c_str()), atof(ele[2].c_str()), atof(ele[3].c_str()));
			break;
		case pS('K', 'e', '*')://emission
			mtl.SetMtl(MY_MODEL_EMISSION, atof(ele[1].c_str()), atof(ele[2].c_str()), atof(ele[3].c_str()));
			break;
		case pS('N','s','*')://shiness
			mtl.SetMtl(MY_MODEL_SHINESS, 0, 0, 0, atof(ele[1].c_str()));
			break;
		case pS('m', 'a', 'p')://Texture
			if (ele[0] == "map_Kd****")
			{
				auto loff = ele[1].find_last_of('\\'),
					roff = ele[1].find_last_of('.');
				ele[1] = ele[1].substr(loff + 1, roff - loff) + "bmp";

				int8_t a = texs.size();
				while (--a >= 0)
					if (texs[a].name == ele[1])
					{
						cur_tex_id = a;
						break;
					}
				if (a < 0)
				{
					loadtex(ele[1], code);
					cur_tex_id = texs.size() - 1;
				}
			}
			break;
		}
	}
	mtls.push_back(mtl);
	mtl_tex.push_back(cur_tex_id);
	return int32_t();
}

int32_t Model::loadtex(const string &texname, const uint8_t code)
{
	BITMAPFILEHEADER FileHeader;    //接受位图文件头
	BITMAPINFOHEADER InfoHeader;    //接受位图信息头
	FILE *fp = fopen(texname.c_str(), "rb");
	if (fp == NULL)
		return -1;
	fread(&FileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
	if (FileHeader.bfType != 0x4d42)  //确保文件是一个位图文件，效验文件类型
	{
		fclose(fp);
		return -1;
	}
	fread(&InfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);
	int32_t width = InfoHeader.biWidth,
		height = InfoHeader.biHeight,
		size = InfoHeader.biSizeImage;
	if (InfoHeader.biSizeImage == 0)          //确保图像数据的大小
		size = width*height * 3;
	fseek(fp, FileHeader.bfOffBits, SEEK_SET);  //将文件指针移动到实际图像数据处
	uint8_t *image = new uint8_t[size];
	fread(image, 1, size, fp);
	//for (int a = 0; a < size; a += 3)
		//swap(image[a], image[a + 2]);

	texs.push_back(move(Texture(texname, width, height, image)));
	delete[] image;
	fclose(fp);
	return int32_t();
}

void Model::reset()
{
	glDeleteTextures(texs.size(), texList);
	memset(texList, 0x0, sizeof(texList));

	VerMin = VerMax = Vertex();

	mtl_tex.swap(vector<int8_t>());
	part_mtl.swap(vector<int8_t>());
	texs.swap(vector<Texture>());
	mtls.swap(vector<Material>());
	parts.swap(vector<vector<Triangle>>());
	//clparts.swap(vector<vector<clTri>>());
	octclparts.swap(vector<vector<clTri>>());
	borders.swap(vector<Vertex>());
	bboxs.swap(vector<Vertex>());

	vers.swap(vector<Vertex>());
	nors.swap(vector<Normal>());
	txcs.swap(vector<Coord2D>());
}

Model::~Model()
{
	reset();
}

int32_t Model::loadOBJ(const wstring &objname, const wstring &mtlname, const uint8_t code)
{
	this->objname = objname, this->mtlname = mtlname;
	reset();
	loadmtl(mtlname, code);
	loadobj(objname, code);
	GLPrepare();
	return 1;
}

void Model::zRotate()
{
	//rotate vertexs
	for (Vertex &v : vers)
	{
		swap(v.y, v.z);
		v.z *= -1;
	}
	//rotate normals
	for (Normal &n : nors)
	{
		swap(n.y, n.z);
		n.z *= -1;
	}
	//rotate all parts
	for (vector<Triangle> &part : parts)
		for (Triangle &t : part)
			for (auto a = 0; a < 3; ++a)
			{
				swap(t.points[a].y, t.points[a].z);
				swap(t.norms[a].y, t.norms[a].z);
				t.points[a].z *= -1;
				t.norms[a].z *= -1;
			}
	//rotate border-box
	for (auto a = 0; a < borders.size(); a += 2)
	{
		Vertex &va = borders[a], &vb = borders[a + 1];
		swap(va.y, va.z); swap(vb.y, vb.z);
		va.z *= -1; vb.z *= -1;
		swap(va.z, vb.z);
	}
	/*for(Vertex &v : borders)
	{
		swap(v.y, v.z);
		v.z *= -1;
	}*/
	swap(VerMin.y, VerMin.z);
	VerMin.z *= -1;
	swap(VerMax.y, VerMax.z);
	VerMax.z *= -1;
	GLPrepare();
}

void Model::SetMtl(const Material & mtl)
{
	for (auto &old : mtls)
		old = mtl;
	//mtls[0] = mtl;
}

void Model::RTPrepare()
{
	BorderMin = VerMin + position, BorderMax = VerMax + position;
	bboxs.clear();
	//for (Vertex &v : borders)
	//bboxs.push_back(v + position);
	//clparts.clear();
	octclparts.clear();
	//vector<clTri> cltpart;
	vector<clTri> octcltpart[8];
	int bbnum = 0;
	for (auto cnta = 0; cnta < parts.size(); ++cnta)
	//for (vector<Triangle> &part : parts)
	{
		vector<Triangle> &part = parts[cnta];

		Vertex va = borders[bbnum], vb = borders[bbnum + 1];
		bboxs.push_back(va + position), bboxs.push_back(vb + position);
		va = (va + vb) * 0.5;
		bbnum += 2;
		for (auto cntb = 0; cntb < part.size(); ++cntb)
		//for (Triangle &t : part)
		{
			Triangle &t = part[cntb];
			clTri clt(t.points[1] - t.points[0], t.points[2] - t.points[0], t.points[0] + position);
			clt.numa = cnta, clt.numb = cntb;

			//cltpart.push_back(clt);
			__m128 tmin = _mm_min_ps(t.points[0].dat, _mm_min_ps(t.points[1].dat, t.points[2].dat));
			__m128 tmax = _mm_max_ps(t.points[0].dat, _mm_max_ps(t.points[1].dat, t.points[2].dat));
			if (tmin.m128_f32[0] <= va.x)
			{
				if (tmin.m128_f32[2] <= va.z)
				{
					if (tmin.m128_f32[1] <= va.y)
						octcltpart[0].push_back(clt);
					if (tmax.m128_f32[1] >= va.y)
						octcltpart[1].push_back(clt);
				}
				if (tmax.m128_f32[2] >= va.z)
				{
					if (tmin.m128_f32[1] <= va.y)
						octcltpart[2].push_back(clt);
					if (tmax.m128_f32[1] >= va.y)
						octcltpart[3].push_back(clt);
				}
			}
			if (tmax.m128_f32[0] >= va.x)
			{
				if (tmin.m128_f32[2] <= va.z)
				{
					if (tmin.m128_f32[1] <= va.y)
						octcltpart[4].push_back(clt);
					if (tmax.m128_f32[1] >= va.y)
						octcltpart[5].push_back(clt);
				}
				if (tmax.m128_f32[2] >= va.z)
				{
					if (tmin.m128_f32[1] <= va.y)
						octcltpart[6].push_back(clt);
					if (tmax.m128_f32[1] >= va.y)
						octcltpart[7].push_back(clt);
				}
			}
		}
		//cltpart.shrink_to_fit();
		//clparts.push_back(move(cltpart));
		//cltpart.reserve(3000);
		for (auto b = 0; b < 8; ++b)
		{
			octcltpart[b].push_back(clTri());
			octcltpart[b].shrink_to_fit();
			octclparts.push_back(move(octcltpart[b]));
			octcltpart[b].reserve(500);
		}
	}
	return;

}

static float BorderTestEx(const Ray & ray, const Vertex &Min, const Vertex &Max, __m256i *mask)
{
	const Vertex Mid = (Min + Max) * 0.5;
	Vertex tmin = Min - ray.origin, tmax = Max - ray.origin, tmid = Mid - ray.origin;
	Vertex rrd = _mm_div_ps(_mm_set1_ps(1.0f), ray.direction);
	tmin = tmin.mixmul(rrd);
	tmax = tmax.mixmul(rrd);
	tmid = tmid.mixmul(rrd);

	__m256 min_mid, mid_max, xa, xb, ya, yb, za, zb;
	min_mid = _mm256_set_m128(tmid, tmin);
	mid_max = _mm256_set_m128(tmax, tmid);
	xa = _mm256_permute_ps(min_mid, 0x00);
	xb = _mm256_permute_ps(mid_max, 0x00);
	ya = _mm256_permutevar8x32_ps(min_mid, _mm256_set_epi32(5, 1, 5, 1, 5, 1, 5, 1));
	yb = _mm256_permutevar8x32_ps(mid_max, _mm256_set_epi32(5, 1, 5, 1, 5, 1, 5, 1));
	za = _mm256_permutevar8x32_ps(min_mid, _mm256_set_epi32(6, 6, 2, 2, 6, 6, 2, 2));
	zb = _mm256_permutevar8x32_ps(mid_max, _mm256_set_epi32(6, 6, 2, 2, 6, 6, 2, 2));

	__m256 txansmin = _mm256_min_ps(xa, xb),
		txansmax = _mm256_max_ps(xa, xb),
		tyansmin = _mm256_min_ps(ya, yb),
		tyansmax = _mm256_max_ps(ya, yb),
		tzansmin = _mm256_min_ps(za, zb),
		tzansmax = _mm256_max_ps(za, zb);

	//test y
	if (abs(ray.direction.y) < 1e-6)
	{
		if (ray.origin.y > Max.y || ray.origin.y < Min.y)
			return 1e20f;
		if (ray.origin.y > Mid.y)
			tyansmin = _mm256_setr_ps(1e20f, 0, 1e20f, 0, 1e20f, 0, 1e20f, 0),
			tyansmax = _mm256_setr_ps(0, 1e20f, 0, 1e20f, 0, 1e20f, 0, 1e20f);
		else if(ray.origin.y < Mid.y)
			tyansmin = _mm256_setr_ps(0, 1e20f, 0, 1e20f, 0, 1e20f, 0, 1e20f),
			tyansmax = _mm256_setr_ps(1e20f, 0, 1e20f, 0, 1e20f, 0, 1e20f, 0);
		else
			tyansmin = _mm256_setzero_ps(),//_mm256_set_ps(0, 0, 0, 0, 0, 0, 0, 0),
			tyansmax = _mm256_set_ps(1e20f, 1e20f, 1e20f, 1e20f, 1e20f, 1e20f, 1e20f, 1e20f);
		
	}
	//test x
	if (abs(ray.direction.x) < 1e-6)
	{
		if (ray.origin.x > Max.x || ray.origin.x < Min.x)
			return 1e20f;
		if (ray.origin.x > Mid.x)
			txansmin = _mm256_setr_ps(1e20f, 1e20f, 1e20f, 1e20f, 0, 0, 0, 0),
			txansmax = _mm256_setr_ps(0, 0, 0, 0, 1e20f, 1e20f, 1e20f, 1e20f);
		else if(ray.origin.x < Mid.x)
			txansmin = _mm256_setr_ps(0, 0, 0, 0, 1e20f, 1e20f, 1e20f, 1e20f),
			txansmax = _mm256_setr_ps(1e20f, 1e20f, 1e20f, 1e20f, 0, 0, 0, 0);
		else
			txansmin = _mm256_setzero_ps(),//_mm256_set_ps(0, 0, 0, 0, 0, 0, 0, 0),
			txansmax = _mm256_set_ps(1e20f, 1e20f, 1e20f, 1e20f, 1e20f, 1e20f, 1e20f, 1e20f);
	}
	//test z
	if (abs(ray.direction.z) < 1e-6)
	{
		if (ray.origin.z > Max.z || ray.origin.z < Min.z)
			return 1e20f;
		if (ray.origin.z > Mid.z)
			tzansmin = _mm256_setr_ps(1e20f, 1e20f, 0, 0, 1e20f, 1e20f, 0, 0),
			tzansmax = _mm256_setr_ps(0, 0, 1e20f, 1e20f, 0, 0, 1e20f, 1e20f);
		else if (ray.origin.z < Mid.z)
			tzansmin = _mm256_setr_ps(0, 0, 1e20f, 1e20f, 0, 0, 1e20f, 1e20f),
			tzansmax = _mm256_setr_ps(1e20f, 1e20f, 0, 0, 1e20f, 1e20f, 0, 0);
		else
			tzansmin = _mm256_setzero_ps(),//_mm256_set_ps(0, 0, 0, 0, 0, 0, 0, 0),
			tzansmax = _mm256_set_ps(1e20f, 1e20f, 1e20f, 1e20f, 1e20f, 1e20f, 1e20f, 1e20f);
	}

	__m256 ansmin = _mm256_max_ps(txansmin, tyansmin),
		ansmax = _mm256_min_ps(txansmax, tyansmax),
		ttansmin = _mm256_max_ps(tzansmin, _mm256_setzero_ps());
	ansmin = _mm256_max_ps(ansmin, ttansmin);
	ansmax = _mm256_min_ps(ansmax, tzansmax);

	
	__m256i state = *(__m256i*)&_mm256_cmp_ps(ansmin, ansmax, _CMP_LE_OS);
	*mask = state;
	float minist = 1e20f;
#pragma region dataspy
	/*if (ansmax.m256_f32[7] < ansmin.m256_f32[7] && ansmax.m256_f32[3] > ansmin.m256_f32[3] && false)
	{
		printf("catch you\n");

		printf("\nMin:\t");
		for (auto k : Min.dat.m128_f32)
			printf("%.2ef\t,", k);
		printf("\nMid:\t");
		for (auto k : Mid.dat.m128_f32)
			printf("%.2ef\t,", k);
		printf("\nMax:\t");
		for (auto k : Max.dat.m128_f32)
			printf("%.2ef\t,", k);
		printf("\nrrd:\t");
		for (auto k : rrd.dat.m128_f32)
			printf("%.2ef\t,", k);
		printf("\ntMin:\t");
		for (auto k : tmin.dat.m128_f32)
			printf("%.2ef\t,", k);
		printf("\ntMid:\t");
		for (auto k : tmid.dat.m128_f32)
			printf("%.2ef\t,", k);
		printf("\ntMax:\t");
		for (auto k : tmax.dat.m128_f32)
			printf("%.2ef\t,", k);

		printf("\n\nmin-mid:\t");
		for (auto k : min_mid.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nmid-max:\t");
		for (auto k : mid_max.m256_f32)
			printf("%.2ef\t,", k);

		printf("\n\nxa:\t");
		for (auto k : xa.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nxb:\t");
		for (auto k : xb.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nya:\t");
		for (auto k : ya.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nyb:\t");
		for (auto k : yb.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nza:\t");
		for (auto k : za.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nzb:\t");
		for (auto k : zb.m256_f32)
			printf("%.2ef\t,", k);

		printf("\n\nxmin:\t");
		for (auto k : txansmin.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nxmax:\t");
		for (auto k : txansmax.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nymin:\t");
		for (auto k : tyansmin.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nymax:\t");
		for (auto k : tyansmax.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nzmin:\t");
		for (auto k : tzansmin.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nzmax:\t");
		for (auto k : tzansmax.m256_f32)
			printf("%.2ef\t,", k);

		printf("\n\nansmin:\t");
		for (auto k : ansmin.m256_f32)
			printf("%.2ef\t,", k);
		printf("\nansmax:\t");
		for (auto k : ansmax.m256_f32)
			printf("%.2ef\t,", k);

		printf("\nresult:\t");
		for (auto k : state.m256i_i32)
			printf("%6s\t,", k ? "true" : "false");
		printf("\nover\n");
	}*/
#pragma endregion dataspy code to show data in progress
	//memcpy(mask, &state, 32);
	for (auto a = 0; a < 8; ++a)
	{
		/*Vertex ttmin(a & 0x4 ? Mid.x : Min.x, a & 0x1 ? Mid.y : Min.y, a & 0x2 ? Mid.z : Min.z);
		Vertex ttmax(a & 0x4 ? Max.x : Mid.x, a & 0x1 ? Max.y : Mid.y, a & 0x2 ? Max.z : Mid.z);
		float empty;
		float ttans = BorderTest(ray, ttmin, ttmax, &empty);*/
		//save[a] = ansmin.m256_f32[a];
		//save[a + 8] = ansmax.m256_f32[a];
		if (state.m256i_i32[a])//min<=max
			minist = min(minist, ansmin.m256_f32[a]);

	}
	return minist;
}

inline static float TriangleTest(const Ray & ray, const clTri & tri, Vertex &coord)
{
#pragma region avx_triangletest
#ifdef AVX_

	__m256 mg14 = _mm256_set_m128(ray.direction, tri.axisv),
		mg23 = _mm256_set_m128(tri.axisv, ray.direction);
	__m256 mg23bak = mg23;
	__m256 t14 = _mm256_permute_ps(mg14, _MM_SHUFFLE(3, 0, 2, 1)),
		t23 = _mm256_permute_ps(mg23, _MM_SHUFFLE(3, 1, 0, 2));
	__m256 lr = _mm256_mul_ps(t14, t23);//l~r->tmp1
	__m128 *r = (__m128*)&lr.m256_f32[0], *l = (__m128*)&lr.m256_f32[4];

	__m256 dpa = _mm256_set_m128(_mm_sub_ps(*l, *r), _mm_sub_ps(*l, *r));//tmp1~tmp1
	mg14 = _mm256_set_m128(_mm_sub_ps(ray.origin, tri.p0), tri.axisu);//t2r~axu
	__m256 dpans = _mm256_dp_ps(dpa, mg14, 0b01110001);//tmp1&t2r ~ a(tmp1&axu)

	mg23 = _mm256_set_m128(tri.axisu, _mm_sub_ps(ray.origin, tri.p0));//axu~t2r
	t14 = _mm256_permute_ps(mg14, _MM_SHUFFLE(3, 0, 2, 1)),
		t23 = _mm256_permute_ps(mg23, _MM_SHUFFLE(3, 1, 0, 2));
	lr = _mm256_mul_ps(t14, t23);//l~r->tmp2

	__m256 ansmix = _mm256_insertf128_ps(dpans, _mm_set1_ps(1.0f), 0);//tmp1&t2r ~ 1
	__m256 aexp = _mm256_permute2f128_ps(dpans, dpans, 0x00);//a`a`a`a`a`a`a`a
															 //__m256 dpfm = _mm256_permute2f128_ps(_mm256_set1_ps(1.0f), dpans, 0x20);
	__m256 divans = _mm256_div_ps(ansmix, aexp);//u(tmp1&t2r/a) ~ f(1/a)
												//divans:***u***f
	float u = divans.m256_f32[4], f = divans.m256_f32[0];
	if (u < 0.0f || u > 1.0f)
		return 1e20f;

	dpa = _mm256_set_m128(_mm_sub_ps(*l, *r), _mm_sub_ps(*l, *r));//tmp2~tmp2
	mg23bak;//axv~rdir

	dpans = _mm256_dp_ps(dpa, mg23bak, 0b01110001);//tmp2&axv ~ tmp2&rdir
	aexp = _mm256_permute2f128_ps(divans, divans, 0x00);//f`f`f`f`f`f`f`f
	__m256 mulans = _mm256_mul_ps(dpans, aexp);
	//mulans:***t***v
	float v = mulans.m256_f32[0], t = mulans.m256_f32[4];
	if (v < 0.0f || v > 1.0f || t < 1e-6f)
		return 1e20f;
	else
	{
		coord = Vertex(1.0f - u - v, u, v);
		return t;
	}
#else
#pragma endregion a wrong way to do triangletest using avx2
	/*
	** Point(u,v) = (1-u-v)*p0 + u*p1 + v*p2
	** Ray:Point(t) = o + t*dir
	** o + t*dir = (1-u-v)*p0 + u*p1 + v*p2
	*/

	Vertex tmp1 = ray.direction * tri.axisv;
	Vertex t2r = ray.origin - tri.p0;

	float f = tri.axisu & tmp1;
	//if (abs(f) < 1e-6f)
	//return 1e20f;
	f = 1.0f / f;

	float u = (t2r & tmp1) * f;

	if (u < 0.0f || u > 1.0f)
		return 1e20f;
	Vertex tmp2 = t2r * tri.axisu;
	float v = (ray.direction & tmp2) * f,
		duv = 1 - u - v;
	if (v < 0.0f || duv < 0.0f)
		return 1e20f;
	float t = (tri.axisv & tmp2) * f;
	if (t > 1e-5f)
	{
		coord = Vertex(duv, u, v);
		return t;
	}
	else
		return 1e20f;
#endif
}

HitRes Model::intersect(const Ray &ray, const HitRes &hr, const float min)
{
	float empty;
	__m256i mask;
	float ans = BorderTest(ray, BorderMin, BorderMax, &empty);
	if (ans < hr.distance)//inside and maybe nearer
	{
		ans = hr.distance;
		float newans;
		int objpart = -1;
		Triangle *objt = nullptr;
		clTri *objclt = nullptr;
		Vertex coord, tmpc;
		int pcnt = parts.size();
		for (auto a = 0; a < pcnt; ++a)//each part
			if (BorderTestEx(ray, bboxs[a * 2], bboxs[a * 2 + 1], &mask) < hr.distance)//part may hit
			{
				for (auto b = 0, pcur = a * 8; b < 8; ++b, ++pcur)//each oct-border
				{
					//if(true)
					if (mask.m256i_i32[b])//oct-border may hit
					{
						int tcnt = octclparts[pcur].size() - 1;
						for (auto c = 0; c < tcnt;)//each triangle in oct-border
						{
							clTri &t = octclparts[pcur][c++];
							//early quit
							if (hr.obj == (intptr_t)&t)
								continue;
							_mm_prefetch((char*)&octclparts[pcur][c], _MM_HINT_T0);
							newans = TriangleTest(ray, t, tmpc);
							if (newans < ans)
							{
								objpart = a;
								objclt = &t;
								objt = &parts[t.numa][t.numb];
								ans = newans;
								coord = tmpc;
								if (newans < min)
									goto ____EOS;
							}
						}
					}
				}
			}
	____EOS:
		if (ans < hr.distance)
		{
			HitRes newhr(ans);
			newhr.position = ray.origin + ray.direction * ans;
			newhr.normal = objt->norms[0] * coord.x + objt->norms[1] * coord.y + objt->norms[2] * coord.z;
			newhr.tcoord = objt->tcoords[0] * coord.x + objt->tcoords[1] * coord.y + objt->tcoords[2] * coord.z;
			auto mnum = part_mtl[objpart];
			newhr.mtl = &mtls[mnum];
			newhr.rfr = newhr.mtl->rfr;
			auto tnum = mtl_tex[mnum];
			if (tnum >= 0)
				newhr.tex = &texs[tnum];
			newhr.obj = (intptr_t)objclt;
			return newhr;
		}
	}
	return hr;
}

void Model::GLPrepare()
{
	glDeleteTextures(texs.size(), texList);
	glGenTextures(texs.size(), texList);
	for (auto a = 0; a < texs.size(); ++a)
	{
		Texture &tex = texs[a];
		glBindTexture(GL_TEXTURE_2D, texList[a]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.w, tex.h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, tex.data);
		//gluBuild2DMipmaps(GL_TEXTURE_2D, 3, tex.w, tex.h, GL_RGB, GL_UNSIGNED_BYTE, tex.data);
	}

	glNewList(GLListNum, GL_COMPILE);
	for (auto a = 0; a < parts.size(); ++a)
	{
		int8_t mnum = part_mtl[a];
		Material &mat = mtls[mnum];
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat.ambient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat.diffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat.specular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat.emission);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat.shiness);

		int8_t tnum = mtl_tex[mnum];
		if (tnum >= 0)
			glBindTexture(GL_TEXTURE_2D, texList[tnum]);
		else
			glBindTexture(GL_TEXTURE_2D, 0);
		glBegin(GL_TRIANGLES);
		for (Triangle &tri : parts[a])
		{
			glTexCoord2fv(tri.tcoords[0]);
			glNormal3fv(tri.norms[0]);
			glVertex3fv(tri.points[0]);
			glTexCoord2fv(tri.tcoords[1]);
			glNormal3fv(tri.norms[1]);
			glVertex3fv(tri.points[1]);
			glTexCoord2fv(tri.tcoords[2]);
			glNormal3fv(tri.norms[2]);
			glVertex3fv(tri.points[2]);
		}
		glEnd();
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	glEndList();
}







Model::Loader::Loader(const wstring &fname)
{
	filename = fname;
	fp = _wfopen(filename.c_str(), L"r");
}

Model::Loader::~Loader()
{
	if (fp != NULL)
		fclose(fp);
}

int8_t Model::Loader::read(string data[])
{
	if (fp == NULL)
		return INT8_MIN;
	if (fgets(tmpdat[0], 256, fp) != NULL)
	{
		auto a = sscanf(tmpdat[0], "%s%s%s%s%s", tmpdat[1], tmpdat[2], tmpdat[3], tmpdat[4], tmpdat[5]);
		strcat(tmpdat[1], "****");
		for (auto b = 0; b < a; ++b)
			data[b] = string(tmpdat[b + 1]);
		return a;
	}
	else
		return INT8_MIN;
}

int8_t Model::Loader::parseFloat(const string &in, float out[])
{
	auto a = sscanf(in.c_str(), "%f/%f/%f/%f", &out[0], &out[1], &out[2], &out[3]);
	return a;
}

int8_t Model::Loader::parseInt(const string &in, int out[])
{
	auto a = sscanf(in.c_str(), "%d/%d/%d/%d", &out[0], &out[1], &out[2], &out[3]);
	if (a < 2)
		a = sscanf(in.c_str(), "%d//%d/%d", &out[0], &out[2], &out[3]);
	return a;
}
