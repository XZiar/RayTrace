#include "Model.h"
#define pS(a,b,c) (a)*65536+(b)*256+(c)
static inline int32_t parseStr(string str)
{
	if (str.length() < 3)
		str += "****";
	return str[2] + 256 * str[1] + 65536 * str[0];
}
static inline void MinMax(GLdouble *minmax, GLdouble *v)
{
	for (auto a = 0, b = 0; a < 3; ++a, b += 2)
	{
		if (v[a] > minmax[b])
			minmax[b] = v[a];
		else if (v[a] < minmax[b + 1])
			minmax[b + 1] = v[a];
	}
}

int32_t Model::loadobj(const wstring &objname, uint8_t code)
{
	Loader ldr(objname);
	string ele[5];
	int8_t num, pn;
	int ti[16];

	bool bZroll = (code & MY_MODEL_Z_ROLL),
		bFirstT = true,
		bFirstO = true;

	//clean
	
	vers.reserve(8000);
	nors.reserve(8000);
	txcs.reserve(8000);
	//load
	GLdouble minmax[6] = { 0.0,0.0,0.0,0.0,0.0,0.0 };
	vector<Triangle> tris;
	tris.reserve(40);
	Vertex v;
	Normal n;
	Vertex tc;
	Triangle t;
	vers.push_back(v);
	nors.push_back(n);
	txcs.push_back(tc);
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
			if (bZroll)
				v = Vertex(atof(ele[1].c_str()), atof(ele[3].c_str()), -1 * atof(ele[2].c_str()));
			else
				v = Vertex(atof(ele[1].c_str()), atof(ele[2].c_str()), atof(ele[3].c_str()));
			MinMax(minmax, v);
			vers.push_back(v);
			break;
		case pS('v', 'n', '*')://normal
			if (bZroll)
				n = Normal(atof(ele[1].c_str()), atof(ele[3].c_str()), -1 * atof(ele[2].c_str()));
			else
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
				break;
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
				break;
			}
			
		case pS('u', 's', 'e')://object part
			if (!bFirstO)
			{
				objs.push_back(tris);
				tris.clear();
				tris.reserve(4000);
			}
			bFirstO = false;
			int8_t a = mtls.size();
			while (--a > 0)
				if (mtls[a].name == ele[1])
					break;
			obj_mtl.push_back(a);
			break;
		}
	}
	{
		GLdouble mdif = minmax[0] - minmax[1],
			mdifa = minmax[2] - minmax[3],
			mdifb = minmax[4] - minmax[5];
		if (mdifa > mdif)
			mdif = mdifa;
		if (mdifb > mdif)
			mdif = mdifb;
		mdif /= 8.0;
		for (vector<Triangle> &vt : objs)
		{
			for(Triangle &t : vt)
				for(Vertex &v : t.points)
					v.x /= mdif, v.y /= mdif, v.z /= mdif;
		}
	}
	if (bFirstO)
	{
		obj_mtl.push_back(0);
	}
	objs.push_back(tris);
	return objs.size();
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

	/*mtl_tex.clear();
	obj_mtl.clear();
	texs.clear();
	mtls.clear();
	objs.clear();

	vers.clear();
	nors.clear();
	txcs.clear();*/

	mtl_tex.swap(vector<int8_t>());
	obj_mtl.swap(vector<int8_t>());
	texs.swap(vector<Texture>());
	mtls.swap(vector<Material>());
	objs.swap(vector<vector<Triangle>>());

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

HitRes Model::intersect(Ray &ray)
{
	return HitRes();
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
	for (auto a = 0; a < objs.size(); ++a)
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
		for (Triangle &tri : objs[a])
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
