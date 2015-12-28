#include "Model.h"
#define pS(a,b,c) (a)*65536+(b)*256+(c)
static inline int32_t parseStr(string str)
{
	if (str.length() < 3)
		str += "****";
	return str[2] + 256 * str[1] + 65536 * str[0];
}

int32_t Model::loadobj(const wstring &objname, uint8_t code)
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
	tris.reserve(4000);
	Vertex v;
	Normal n;
	Vertex tc;
	Triangle t;
	vers.push_back(v);
	nors.push_back(n);
	txcs.push_back(tc);

	VerMin = Vertex(1000, 1000, 1000);
	VerMax = Vertex(-1000, -1000, -1000);

	auto Border = [](Vertex &Min, Vertex &Max, const Vertex &v)
	{
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
			tc = Vertex(atof(ele[1].c_str()), atof(ele[2].c_str()), atof(ele[3].c_str()));
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
				Border(VerMin, VerMax, vers[ti[0]]);
				Border(VerMin, VerMax, vers[ti[3]]);
				Border(VerMin, VerMax, vers[ti[6]]);
			}
			break;
		case pS('u', 's', 'e')://object part
			if (!bFirstO)
			{
				parts.push_back(tris);
				tris.clear();
				borders.push_back(VerMin);
				borders.push_back(VerMax);
				//tris.reserve(4000);
			}
			VerMin = Vertex(1000, 1000, 1000);
			VerMax = Vertex(-1000, -1000, -1000);
			bFirstO = false;
			int8_t a = mtls.size();
			while (--a > 0)
				if (mtls[a].name == ele[1])
					break;
			obj_mtl.push_back(a);
			break;
		}
	}
	if (bFirstO)
	{
		obj_mtl.push_back(0);
	}
	parts.push_back(tris);
	tris.swap(vector<Triangle>());
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
		double mdif = Dif.x > Dif.y ? Dif.x : Dif.y;
		mdif = Dif.z > mdif ? Dif.z / 8.0 : mdif / 8.0;
		VerMax /= mdif, VerMin /= mdif;
		//resize borders
		for (Vertex &v : borders)
			v /= mdif;
		//resize triangles
		for (vector<Triangle> &vt : parts)
		{
			for(Triangle &t : vt)
				for(Vertex &v : t.points)
					v /= mdif;
		}
	}
	
	return parts.size();
}

int32_t Model::loadmtl(const wstring &mtlname, uint8_t code)
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
				mtl = Material();
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

int32_t Model::loadtex(const string &texname, uint8_t code)
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

	texs.push_back(Texture(texname, width, height, image));
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
	obj_mtl.swap(vector<int8_t>());
	texs.swap(vector<Texture>());
	mtls.swap(vector<Material>());
	parts.swap(vector<vector<Triangle>>());
	newparts.swap(vector<vector<Triangle>>());
	borders.swap(vector<Vertex>());
	bboxs.swap(vector<Vertex>());

	vers.swap(vector<Vertex>());
	nors.swap(vector<Normal>());
	txcs.swap(vector<Vertex>());
	
}

Model::~Model()
{
	reset();
}

int32_t Model::loadOBJ(const wstring &objname, const wstring &mtlname, uint8_t code)
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
	vector<Triangle> tmppart;
	for (vector<Triangle> &part : parts)
	{
		for (Triangle &t : part)
		{
			Triangle newt = t;
			newt.points[0] += position;
			newt.points[1] += position;
			newt.points[2] += position;
			tmppart.push_back(newt);
		}
		newparts.push_back(tmppart);
		tmppart.clear();
	}
}

HitRes Model::intersect(const Ray &ray, const HitRes &hr)
{
	double ans = BorderTest(ray, BorderMin, BorderMax);
	if (ans < 1e15)//inside
	{
		HitRes newhr = hr;
		double newans;
		vector<bool> tpart;
		for (auto a = 0; a < bboxs.size(); a += 2)
		{
			newans = BorderTest(ray, bboxs[a], bboxs[a + 1]);
			tpart.push_back(newans < hr.distance);
		}
		for (auto a = 0; a < tpart.size(); ++a)
			if(tpart[a])
				for (Triangle &t : newparts[a])
				{
					newans = TriangleTest(ray, t);
					if (newhr.distance > newans)
						newhr = HitRes(newans);
				}
		return newhr;
	}
	return hr;
}

void Model::GLPrepare()
{
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
		int8_t mnum = obj_mtl[a];
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
			glTexCoord2dv(tri.tcoords[0]);
			glNormal3dv(tri.norms[0]);
			glVertex3dv(tri.points[0]);
			glTexCoord2dv(tri.tcoords[1]);
			glNormal3dv(tri.norms[1]);
			glVertex3dv(tri.points[1]);
			glTexCoord2dv(tri.tcoords[2]);
			glNormal3dv(tri.norms[2]);
			glVertex3dv(tri.points[2]);
		}
		glEnd();
	}
	glEndList();
}

double Model::TriangleTest(const Ray & ray, const Triangle & tri)
{
	/*
	** Point(u,v) = (1-u-v)*p0 + u*p1 + v*p2
	** Ray:Point(t) = o + t*dir
	** o + t*dir = (1-u-v)*p0 + u*p1 + v*p2
	*/
	Vertex axisu = tri.points[1] - tri.points[0],
		axisv = tri.points[2] - tri.points[0];
	Vertex tmp1 = ray.direction * axisv;
	double a = axisu & tmp1;
	if (abs(a) < 1e-6)
		return 1e20;
	double f = 1 / a;
	Vertex t2r = ray.origin - tri.points[0];
	double u = (t2r & tmp1) * f;
	if (u < 0.0 || u > 1.0)
		return 1e20;
	Vertex tmp2 = t2r * axisu;
	double v = (ray.direction & tmp2) * f;
	if (v < 0.0 || u + v>1.0)
		return 1e20;
	double t = (axisv & tmp2) * f;
	if (t > 1e-6)
		return t;
	else
		return 1e20;
}





Model::Loader::Loader(wstring filename)
{
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

int8_t Model::Loader::parseDouble(const string &in, GLdouble out[])
{
	auto a = sscanf(in.c_str(), "%lf/%lf/%lf/%lf", &out[0], &out[1], &out[2], &out[3]);
	return a;
}

int8_t Model::Loader::parseInt(const string &in, int out[])
{
	auto a = sscanf(in.c_str(), "%d/%d/%d/%d", &out[0], &out[1], &out[2], &out[3]);
	if (a < 2)
		a = sscanf(in.c_str(), "%d//%d/%d", &out[0], &out[2], &out[3]);
	return a;
}
