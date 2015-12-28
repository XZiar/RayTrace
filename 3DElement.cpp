#include "3DElement.h"

static double mod(const double &l, const double &r)
{
	double t, e;
	modf(l / r, &t);
	e = t*r;
	return l - e;
}

inline void Coord_sph2car2(double &angy, double &angz, const double dis, Vertex &v)
{
	v.z = dis * sin(angy*PI / 180) * cos(angz*PI / 180.0);
	v.x = dis * sin(angy*PI / 180) * sin(angz*PI / 180);
	v.y = dis * cos(angy*PI / 180);
}

inline void Coord_sph2car(double &angy, double &angz, const double dis, Vertex &v)
{
	bool fix = false;
	if (angz >= 180)
		angz = mod(angz, 180), angy = mod(360 - angy, 360), fix = true;
	if (angy < 1e-6)
		angy = 360;
	v.z = dis * sin(angy*PI / 180) * cos(angz*PI / 180.0);
	v.x = dis * sin(angy*PI / 180) * sin(angz*PI / 180);
	if (fix && mod(angy, 180) < 1e-6)
		v.z *= -1, v.x *= -1;
	v.y = dis * cos(angy*PI / 180);
}

inline void Coord_car2sph(const Vertex &v, double &angy, double &angz, double &dis)
{
	dis = v.length();
	//angy = atan2(sqrt(x*x + z*z), y) *180 / PI;
	angy = acos(v.y / dis) * 180 / PI;
	angz = atan2(v.x, v.z) * 180 / PI;
	angy = mod(360 + angy, 360);
	angz = mod(360 + angz, 360);
	/*if (angz >= 180)
		angz = mod(angz, 180), angy = mod(360 - angy, 360);*/
	if (angy < 1e-6)
		angy = 360;
}



Vertex::Vertex()
{
	x = y = z = 0;
}
Vertex::Vertex(GLdouble ix, GLdouble iy, GLdouble iz)
{
	x = ix;
	y = iy;
	z = iz;
}
GLdouble Vertex::length() const
{
	return sqrt(x*x + y*y + z*z);
}
GLdouble Vertex::length_sqr() const
{
	return x*x + y*y + z*z;
}
Vertex Vertex::operator+(const Vertex &v)
{
	return Vertex(x + v.x, y + v.y, z + v.z);
}
Vertex &Vertex::operator+=(const Vertex & right)
{
	x += right.x, y += right.y, z += right.z;
	return *this;
}
Vertex Vertex::operator-(const Vertex &v)
{
	return Vertex(x - v.x, y - v.y, z - v.z);
}
Vertex &Vertex::operator-=(const Vertex & right)
{
	x += right.x, y += right.y, z += right.z;
	return *this;
}
Vertex Vertex::operator/(const double &n)
{
	return Vertex(x / n, y / n, z / n);
}
Vertex Vertex::operator*(const double &n)
{
	return Vertex(x * n, y * n, z * n);
}
Vertex Vertex::operator*(const Vertex &v)
{
	GLdouble a, b, c;
	a = y*v.z - z*v.y;
	b = z*v.x - x*v.z;
	c = x*v.y - y*v.x;
	return Vertex(a, b, c);
}
GLdouble Vertex::operator&(const Vertex & v)
{
	return x*v.x + y*v.y + z*v.z;
}
Vertex::operator GLdouble*()
{
	return &x;
}
Vertex::operator GLfloat*()
{
	fx = GLfloat(x);
	fy = GLfloat(y);
	fz = GLfloat(z);
	return &fx;
}



Normal::Normal(Vertex v)//��һ��
{
	GLdouble s = v.x*v.x + v.y*v.y + v.z*v.z;
	s = sqrt(s);
	x = v.x / s;
	y = v.y / s;
	z = v.z / s;
}



Texture::Texture(const string &iname, const int16_t iw, const int16_t ih, const uint8_t * img)
{
	name = iname;
	w = iw, h = ih;
	int32_t size = w*h * 3;
	if (data != nullptr)
	{
		delete[] data;
		data = nullptr;
	}
	data = new uint8_t[size];
	memcpy(data, img, size);
}

Texture::~Texture()
{
	if (data != nullptr)
	{
		delete[] data;
		data = nullptr;
	}
}

Texture::Texture(const Texture & t)
{
	name = t.name;
	w = t.w, h = t.h;
	int32_t size = w*h * 3;
	if (data != nullptr)
	{
		delete[] data;
		data = nullptr;
	}
	if (t.data != nullptr)
	{
		data = new uint8_t[size];
		memcpy(data, t.data, size);
	}
}



Material::Material()
{
	name = "simple";
	SetMtl(MY_MODEL_AMBIENT | MY_MODEL_DIFFUSE, 0.588f, 0.588f, 0.588f);
	SetMtl(MY_MODEL_EMISSION | MY_MODEL_SPECULAR, 0.0f, 0.0f, 0.0f);
	SetMtl(MY_MODEL_SHINESS, 10.0f, 10.0f, 10.0f);
}

Material::~Material()
{
}

void Material::SetMtl(int8_t prop, GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	if (prop & MY_MODEL_AMBIENT)
		ambient[0] = r, ambient[1] = g, ambient[2] = b, ambient[3] = a;
	if (prop & MY_MODEL_DIFFUSE)
		diffuse[0] = r, diffuse[1] = g, diffuse[2] = b, diffuse[3] = a;
	if (prop & MY_MODEL_SHINESS)
		shiness[0] = r, shiness[1] = g, shiness[2] = b, shiness[3] = a;
	if (prop & MY_MODEL_EMISSION)
		emission[0] = r, emission[1] = g, emission[2] = b, emission[3] = a;
	if (prop & MY_MODEL_SPECULAR)
		specular[0] = r, specular[1] = g, specular[2] = b, specular[3] = a;
}



Triangle::Triangle()
{
}

Triangle::Triangle(Vertex va, Vertex vb, Vertex vc)
{
	points[0] = va, points[1] = vb, points[2] = vc;
	norms[0] = Normal(), norms[1] = Normal(), norms[2] = Normal();
	tcoords[0] = Vertex(), tcoords[1] = Vertex(), tcoords[2] = Vertex();
}

Triangle::Triangle(Vertex va, Normal na, Vertex vb, Normal nb, Vertex vc, Normal nc)
{
	points[0] = va, points[1] = vb, points[2] = vc;
	norms[0] = na, norms[1] = nb, norms[2] = nc;
	tcoords[0] = Vertex(), tcoords[1] = Vertex(), tcoords[2] = Vertex();
}

Triangle::Triangle(Vertex va, Normal na, Vertex ta, Vertex vb, Normal nb, Vertex tb, Vertex vc, Normal nc, Vertex tc)
{
	points[0] = va, points[1] = vb, points[2] = vc;
	norms[0] = na, norms[1] = nb, norms[2] = nc;
	tcoords[0] = ta, tcoords[1] = tb, tcoords[2] = tc;
}



Color::Color(bool black)
{
	if (black)
		r = g = b = 0;
	else
		r = g = b = 255;
}

void Color::put(uint8_t * addr)
{
	*addr = r, *(addr + 1) = g, *(addr + 2) = b;
}

void Color::get(uint8_t * addr)
{
	r = *addr, g = *(addr + 1), b = *(addr + 2);
}



HitRes::operator bool()
{
	return isHit;
}



void DrawObject::GLDraw()
{
	glCallList(GLListNum);
}



Sphere::Sphere(double r, GLuint lnum) : DrawObject(lnum)
{
	radius = r;
	radius_sqr = r * r;
}

void Sphere::SetMtl(const Material & mtl)
{
	this->mtl = mtl;
}

void Sphere::GLPrepare()
{
	glNewList(GLListNum, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mtl.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mtl.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mtl.specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mtl.shiness);
	glMaterialfv(GL_FRONT, GL_EMISSION, mtl.emission);
	glutSolidSphere(radius, 100, 100);
	glEndList();
}

HitRes Sphere::intersect(Ray &ray)
{
	/*
	** s2r->vector that sphere's origin towards ray
	** t = -d.s2r-sqrt[(d.s2r)^2-(s2r^2-r^2)]
	*/
	Vertex s2r = position - ray.origin;
	double rdDOTr2s = ray.direction & s2r;
	if (rdDOTr2s > 0)
		return false;
	double dis = rdDOTr2s * rdDOTr2s - s2r.length_sqr() + radius_sqr;
	if (dis < 0)
		return false;
	GLdouble t = -((ray.direction & s2r) + sqrt(dis));
	return t;
}



Light::Light(int8_t type)
{
	bLight = true;
	rangy = 90, rangz = 0, rdis = 8;
	move(0, 0, 0);
	SetProp(MY_MODEL_AMBIENT, 0.05f, 0.05f, 0.05f);
	SetProp(MY_MODEL_DIFFUSE | MY_MODEL_SPECULAR, 1.0f, 1.0f, 1.0f);
	switch (type)
	{
	case MY_MODEL_LIGHT_PARALLEL:
		position[3] = 0.0f;
		attenuation[0] = attenuation[1] = attenuation[2] = 0;
		break;
	case MY_MODEL_LIGHT_POINT:
		position[3] = 1.0f;
		SetProp(MY_MODEL_ATTENUATION, -0.008f, 0.004f, 0.00005f);
		break;
	case MY_MODEL_LIGHT_SPOT:
		position[3] = 1.0f;
		break;
	}
}

bool Light::turn()
{
	return bLight = !bLight;
}

void Light::move(const int8_t &dangy, const int8_t &dangz, const int8_t &ddis)
{
	rdis += ddis;
	if (rdis < 8)
		rdis = 8;
	else if (rdis > 32)
		rdis = 32;
	rangy = mod(360 + rangy + dangy, 360);
	rangz = mod(360 + rangz + dangz, 360);
	angy = rangy, angz = rangz, dis = rdis;

	Vertex pos;
	Coord_sph2car(angy, angz, dis, pos);
	position[0] = pos.x, position[1] = pos.y, position[2] = pos.z;
}

void Light::SetProp(int16_t prop, GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	if (prop & MY_MODEL_AMBIENT)
		ambient[0] = r, ambient[1] = g, ambient[2] = b, ambient[3] = a;
	if (prop & MY_MODEL_DIFFUSE)
		diffuse[0] = r, diffuse[1] = g, diffuse[2] = b, diffuse[3] = a;
	if (prop & MY_MODEL_SPECULAR)
		specular[0] = r, specular[1] = g, specular[2] = b, specular[3] = a;
	if (prop & MY_MODEL_ATTENUATION)
		attenuation[0] = r, attenuation[1] = g, attenuation[2] = b;
	if (prop & MY_MODEL_POSITION)
		position[0] = r, position[1] = g, position[2] = b, position[3] = a;
}



Camera::Camera(GLint w, GLint h)
{
	width = w, height = h;
	aspect = (GLdouble)w / h;
	fovy = 55.0, zNear = 1.0, zFar = 100.0;

	position = Vertex(0, 0, 15);
	u = Vertex(1, 0, 0);
	v = Vertex(0, 1, 0);
	n = Vertex(0, 0, -1);
}

void Camera::move(const double & x, const double & y, const double & z)
{
	position += u*x;
	position += v*y;
	position += n*z;
}

void Camera::yaw(const double angz)
{
	//rotate n(toward)
	double oangy = acos(n.y / 1) * 180 / PI,
	oangz = atan2(n.x, n.z) * 180 / PI;
	oangz -= angz;
	Coord_sph2car2(oangy, oangz, n.length(), n);
	//rotate u(right)
	oangy = acos(u.y / 1) * 180 / PI;
	oangz = atan2(u.x, u.z) * 180 / PI;
	oangz -= angz;
	Coord_sph2car2(oangy, oangz, u.length(), u);
}

void Camera::pitch(double angy)
{
	//rotate n(toward)
	double oangy = acos(n.y / 1) * 180 / PI,
		oangz = atan2(n.x, n.z) * 180 / PI;
	if (oangy - angy < 1.0)
		angy = oangy - 1.0;
	if (oangy - angy > 179.0)
		angy = oangy - 179.0;
	oangy -= angy;
	Coord_sph2car2(oangy, oangz, n.length(), n);

	//rotate v(up)
	oangy = acos(v.y / 1) * 180 / PI,
	oangz = atan2(v.x, v.z) * 180 / PI;
	oangy += angy;
	oangy = abs(oangy);
	if (oangy > 90.0)
		oangy = 90.0;

	Coord_sph2car2(oangy, oangz, v.length(), v);
}

void Camera::resize(GLint w, GLint h)
{
	width = w, height = h;
	aspect = (GLdouble)w / h;
}
