#include "3DElement.h"

static double mod(const double &l, const double &r)
{
	double t, e;
	modf(l / r, &t);
	e = t*r;
	return l - e;
}

inline void Coord_sph2car(double &angy, double &angz, const double dis, Vertex &v)
{
	v.z = dis * sin(angy*PI / 180) * cos(angz*PI / 180.0);
	v.x = dis * sin(angy*PI / 180) * sin(angz*PI / 180);
	v.y = dis * cos(angy*PI / 180);
}

inline void Coord_sph2car2(double &angy, double &angz, const double dis, Vertex &v)
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
	angy = acos(v.y / dis) * 180 / PI;
	angz = atan2(v.x, v.z) * 180 / PI;
}

double BorderTest(const Ray & ray, const Vertex &Min, const Vertex &Max)
{
	Vertex dismin = Min - ray.origin, dismax = Max - ray.origin;
	//test z
	if (ray.direction.z != 0.0)
	{
		dismin.z /= ray.direction.z;
		dismax.z /= ray.direction.z;
		if (dismin.z > dismax.z)
			swap(dismin.z, dismax.z);
	}
	else
	{
		if (ray.origin.z > Max.z || ray.origin.z < Min.z)
			return 1e20;
		dismin.z = -1, dismax.z = 1e10;
	}
	//test y
	if (ray.direction.y != 0.0)
	{
		dismin.y /= ray.direction.y;
		dismax.y /= ray.direction.y;
		if (dismin.y > dismax.y)
			swap(dismin.y, dismax.y);
	}
	else
	{
		if (ray.origin.y > Max.y || ray.origin.y < Min.y)
			return 1e20;
		dismin.y = -1, dismax.y = 1e10;
	}
	//test x
	if (ray.direction.x != 0.0)
	{
		dismin.x /= ray.direction.x;
		dismax.x /= ray.direction.x;
		if (dismin.x > dismax.x)
			swap(dismin.x, dismax.x);
	}
	else
	{
		if (ray.origin.x > Max.x || ray.origin.x < Min.x)
			return 1e20;
		dismin.x = -1, dismax.x = 1e10;
	}

	double dmin = dismin.x < dismin.y ? dismin.y : dismin.x,
		dmax = dismax.x < dismax.y ? dismax.x : dismax.y;
	dmin = dmin < dismin.z ? dismin.z : dmin,
		dmax = dmax < dismax.z ? dmax : dismax.z;
	dmin = dmin < 0.0 ? 0.0 : dmin;
	if (dmax < dmin)
		return 1e20;
	return dmin;
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
Vertex Vertex::operator+(const Vertex &v) const
{
	return Vertex(x + v.x, y + v.y, z + v.z);
}
Vertex &Vertex::operator+=(const Vertex & right)
{
	x += right.x, y += right.y, z += right.z;
	return *this;
}
Vertex Vertex::operator-(const Vertex &v) const
{
	return Vertex(x - v.x, y - v.y, z - v.z);
}
Vertex &Vertex::operator-=(const Vertex & right)
{
	x += right.x, y += right.y, z += right.z;
	return *this;
}
Vertex Vertex::operator/(const double &n) const
{
	return Vertex(x / n, y / n, z / n);
}
Vertex &Vertex::operator/=(const double & right)
{
	x /= right, y /= right, z /= right;
	return *this;
}
Vertex Vertex::operator*(const double &n) const
{
	return Vertex(x * n, y * n, z * n);
}
Vertex Vertex::operator*(const Vertex &v) const
{
	GLdouble a, b, c;
	a = y*v.z - z*v.y;
	b = z*v.x - x*v.z;
	c = x*v.y - y*v.x;
	return Vertex(a, b, c);
}
GLdouble Vertex::operator&(const Vertex & v) const
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



Normal::Normal(Vertex v)//¹éÒ»»¯
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



Color::Color(const bool black)
{
	if (black)
		r = g = b = 0;
	else
		r = g = b = 255;
}

Color::Color(const double depth, const double mindepth, const double maxdepth)
{
	if (depth <= mindepth)
	{
		r = 255, g = b = 0;
		return;
	}
	if (depth >= maxdepth)
	{
		r = g = 0, b = 0;
		return;
	}
	double after = log(depth), max = log(maxdepth);
	r = g = b = (max - after) * 255 / max;
}

Color::Color(const Normal n)
{
	r = 127 * (n.x + 1);
	g = 127 * (n.y + 1);
	b = 127 * (n.z + 1);
}

void Color::put(uint8_t * addr)
{
	*addr = r, *(addr + 1) = g, *(addr + 2) = b;
}

void Color::get(uint8_t * addr)
{
	r = *addr, g = *(addr + 1), b = *(addr + 2);
}



HitRes::HitRes(bool b)
{
	distance = b ? 1e10 : 1e20;
}

bool HitRes::operator<(const HitRes & right)
{
	return distance < right.distance;
}

HitRes::operator bool()
{
	return distance < 1e15;
}



void DrawObject::GLDraw()
{
	glCallList(GLListNum);
}



Sphere::Sphere(const double r, GLuint lnum) : DrawObject(lnum)
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

HitRes Sphere::intersect(const Ray &ray, const HitRes &hr)
{
	/*
	** s2r->vector that sphere's origin towards ray
	** t = -d.s2r-sqrt[(d.s2r)^2-(s2r^2-r^2)]
	*/
	Vertex s2r = ray.origin - position;
	double rdDOTr2s = ray.direction & s2r;
	if (rdDOTr2s > 0)
		return hr;
	double dis = rdDOTr2s * rdDOTr2s - s2r.length_sqr() + radius_sqr;
	if (dis < 0)
		return hr;
	GLdouble t = -((ray.direction & s2r) + sqrt(dis));
	if (t < hr.distance)
	{
		HitRes newhr(t);
		Vertex point = ray.origin + ray.direction * t;
		newhr.normal = Normal(point - position);
		return newhr;
	}
	return hr;
}



Box::Box(const double len, GLuint lnum) : DrawObject(lnum)
{
	width = height = length = len;
	double l = len / 2;
	max = Vertex(l, l, l);
	min = max * -1;
}

Box::Box(const double l, const double w, const double h, GLuint lnum) : DrawObject(lnum)
{
	length = l, width = w, height = h;
	max = Vertex(l / 2, w / 2, h / 2);
	max = max * -1;
}

void Box::SetMtl(const Material & mtl)
{
	this->mtl = mtl;
}

void Box::GLPrepare()
{
	glNewList(GLListNum, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mtl.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mtl.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mtl.specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mtl.shiness);
	glMaterialfv(GL_FRONT, GL_EMISSION, mtl.emission);
	glBegin(GL_QUADS);
	//fornt
	glNormal3d(0, 0, 1);
	glVertex3d(max.x, min.y, max.z);
	glVertex3d(max.x, max.y, max.z);
	glVertex3d(min.x, max.y, max.z);
	glVertex3d(min.x, min.y, max.z);
	//right
	glNormal3d(1, 0, 0);
	glVertex3d(max.x, min.y, max.z);
	glVertex3d(max.x, max.y, max.z);
	glVertex3d(max.x, max.y, min.z);
	glVertex3d(max.x, min.y, min.z);
	//back
	glNormal3d(0, 0, -1);
	glVertex3d(min.x, min.y, min.z);
	glVertex3d(min.x, max.y, min.z);
	glVertex3d(max.x, max.y, min.z);
	glVertex3d(max.x, min.y, min.z);
	//left
	glNormal3d(-1, 0, 0);
	glVertex3d(min.x, min.y, max.z);
	glVertex3d(min.x, max.y, max.z);
	glVertex3d(min.x, max.y, min.z);
	glVertex3d(min.x, min.y, min.z);
	//up
	glNormal3d(0, 1, 0);
	glVertex3d(max.x, max.y, max.z);
	glVertex3d(max.x, max.y, min.z);
	glVertex3d(min.x, max.y, min.z);
	glVertex3d(min.x, max.y, max.z);
	//down
	glNormal3d(0, -1, 0);
	glVertex3d(max.x, min.y, min.z);
	glVertex3d(max.x, min.y, max.z);
	glVertex3d(min.x, min.y, max.z);
	glVertex3d(min.x, min.y, min.z);

	glEnd();
	glEndList();
}

HitRes Box::intersect(const Ray & ray, const HitRes &hr)
{
	double res = BorderTest(ray, min + position, max + position);
	if (hr.distance > res)
	{
		HitRes newhr(res);
		Vertex point = ray.origin + ray.direction * res;
		Vertex b2p = point - position;
		point.x = point.y = point.z = 0;
		if (abs(abs(b2p.z) - max.z) < 1e-6)//front or back
			point.z = b2p.z>0 ? 1 : -1;
		if (abs(abs(b2p.y) - max.y) < 1e-6)//up or down
			point.y = b2p.y>0 ? 1 : -1;
		if (abs(abs(b2p.x) - max.x) < 1e-6)//left or right
			point.x = b2p.x>0 ? 1 : -1;
		newhr.normal = Normal(point);
		return newhr;
	}
	else
		return hr;
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
	Coord_sph2car2(angy, angz, dis, pos);
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
	fovy = 45.0, zNear = 1.0, zFar = 100.0;

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
	Coord_sph2car(oangy, oangz, 1, n);
	//rotate u(right)
	oangy = acos(u.y / 1) * 180 / PI;
	oangz = atan2(u.x, u.z) * 180 / PI;
	oangz -= angz;
	Coord_sph2car(oangy, oangz, 1, u);
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
	Coord_sph2car(oangy, oangz, 1, n);

	//rotate v(up)
	oangy = acos(v.y / 1) * 180 / PI,
	oangz = atan2(v.x, v.z) * 180 / PI;
	oangy += angy;
	oangy = abs(oangy);
	if (oangy > 90.0)
		oangy = 90.0;

	Coord_sph2car(oangy, oangz, 1, v);
}

void Camera::resize(GLint w, GLint h)
{
	width = w, height = h;
	aspect = (GLdouble)w / h;
}


