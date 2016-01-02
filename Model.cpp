#include "rely.h"
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
		case pS('K', 's', '*')://shiness
			mtl.SetMtl(MY_MODEL_SHINESS, atof(ele[1].c_str()), atof(ele[2].c_str()), atof(ele[3].c_str()));
			break;
		case pS('K', 'e', '*')://emission
			mtl.SetMtl(MY_MODEL_EMISSION, atof(ele[1].c_str()), atof(ele[2].c_str()), atof(ele[3].c_str()));
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
	newparts.swap(vector<vector<Triangle>>());
	borders.swap(vector<Vertex>());
	bboxs.swap(vector<Vertex>());

	vers.swap(vector<Vertex>());
	nors.swap(vector<Normal>());
	txcs.swap(vector<Coord2D>());
}

Model::Model(uint32_t num) : DrawObject(num)
{ 
	type = MY_OBJECT_MODEL; 
};

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
	for(Vertex &v : borders)
	{
		swap(v.y, v.z);
		v.z *= -1;
	}
	swap(VerMin.y, VerMin.z);
	VerMin.z *= -1;
	swap(VerMax.y, VerMax.z);
	VerMax.z *= -1;
	GLPrepare();
}

void Model::RTPrepare()
{
	BorderMin = VerMin + position, BorderMax = VerMax + position;
	bboxs.clear();
	for (Vertex &v : borders)
		bboxs.push_back(v + position);
	newparts.clear();
	int cur = 0;
	vector<ampTri> tpart;
	vector<Triangle> tmppart;
	for (vector<Triangle> &part : parts)
	{
		for (Triangle &t : part)
		{
			Triangle newt = t;
			newt.points[0] += position;
			newt.points[1] += position;
			newt.points[2] += position;
			newt.axisu = newt.points[1] - newt.points[0];
			newt.axisv = newt.points[2] - newt.points[0];
			tmppart.push_back(newt);

			tpart.push_back(newt);
		}
		newparts.push_back(move(tmppart));
		tmppart.reserve(2000);

		tpart.shrink_to_fit();
		gpuParts[cur++] = array_view<const ampTri, 1>(tpart.size(), tpart);
		tpart.clear();
	}



}

#ifdef USING_CPPAMP

auto ampTriTest = [](const ampRay &ray, const ampTri &tri, ampRes &res) restrict(amp) -> float
{
	
};

HitRes Model::intersect(const Ray &ray, const HitRes &hr, const float min)
{
	float ans = BorderTest(ray, BorderMin, BorderMax);
	if (ans < hr.distance)//inside and maybe nearer
	{
		ampRay ampray;
		v2v(ray.direction, ampray.dir); v2v(ray.origin, ampray.ori);

		ampRes ampres[5000];
		float resdis[5000];
		array_view<ampRes, 1> ares(4000, ampres);
		array_view<float, 1> adis(4000, resdis);
		ares.discard_data(); adis.discard_data();

		ans = hr.distance;
		int objpart = -1;
		Triangle *objt = nullptr;
		Vertex coord;
		for (auto a = 0; a < newparts.size(); ++a)
			if (BorderTest(ray, bboxs[a * 2], bboxs[a * 2 + 1]) < hr.distance)
			{
				size_t size = newparts[a].size();

				parallel_for_each(gpuParts[a].extent, [=](index<1> idx) restrict(amp)
				{
					VEC3 tmp1 = ampray.dir * gpuParts[a][idx].v;
					float tmpa = gpuParts[a][idx].u & tmp1;
					if (fast_math::fabs(tmpa) < 1e-6f)
					{
						adis[idx] = 1e20f; return;
					}
					float f = 1 / a;
					VEC3 t2r = ampray.ori - gpuParts[a][idx].p0;
					float u = (t2r & tmp1) * f;
					if (u < 0.0f || u > 1.0f)
					{
						adis[idx] = 1e20f; return;
					}
					VEC3 tmp2 = t2r * gpuParts[a][idx].u;
					float v = (ampray.dir & tmp2) * f,
						duv = 1 - u - v;
					if (v < 0.0f || duv < 0.0f)
					{
						adis[idx] = 1e20f; return;
					}
					float t = (gpuParts[a][idx].v & tmp2) * f;
					if (t > 1e-5f)
					{
						ares[idx].u = u, ares[idx].v = v;
						adis[idx] = t; return;
					}
					else
					{
						adis[idx] = 1e20f; return;
					}
				});

				for (auto b = 0; b < size; ++b)
					if (adis[b] < ans)
					{
						objpart = a;
						objt = &newparts[a][b];
						ans = adis[b];
						coord = Vertex(1 - ares[b].u - ares[b].v, ares[b].u, ares[b].v);
						if (ans < min)
							goto ____EOS;
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
			auto tnum = mtl_tex[mnum];
			if (tnum >= 0)
				newhr.tex = &texs[tnum];
			newhr.obj = (intptr_t)objt;
			return newhr;
		}
	}
	return hr;
}

#else
HitRes Model::intersect(const Ray &ray, const HitRes &hr, const float min)
{
	float ans = BorderTest(ray, BorderMin, BorderMax);
	if (ans < hr.distance)//inside and maybe nearer
	{
		ans = hr.distance;
		float newans;
		int objpart = -1;
		Triangle *objt = nullptr;
		Vertex coord, tmpc;
		for (auto a = 0; a < newparts.size(); ++a)
			if (BorderTest(ray, bboxs[a * 2], bboxs[a * 2 + 1]) < hr.distance)
				for (Triangle &t : newparts[a])
				{
					//early quit
					if (hr.obj == (intptr_t)&t)
						continue;
					newans = TriangleTest(ray, t, tmpc);
					if (newans < ans && newans > 1e-5)
					{
						objpart = a;
						objt = &t;
						ans = newans;
						coord = tmpc;
						if (newans < min)
							goto ____EOS;
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
			auto tnum = mtl_tex[mnum];
			if (tnum >= 0)
				newhr.tex = &texs[tnum];
			newhr.obj = (intptr_t)objt;
			return newhr;
		}
	}
	return hr;
}
#endif



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
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat.shiness);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat.emission);

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
	glEndList();
}

float Model::TriangleTest(const Ray & ray, const Triangle & tri, Vertex &coord)
{
	/*
	** Point(u,v) = (1-u-v)*p0 + u*p1 + v*p2
	** Ray:Point(t) = o + t*dir
	** o + t*dir = (1-u-v)*p0 + u*p1 + v*p2
	*/
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
