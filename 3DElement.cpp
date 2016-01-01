#include "rely.h"
#include "3DElement.h"

static float mod(const float &l, const float &r)
{
	float t, e;
	modf(l / r, &t);
	e = t*r;
	return l - e;
}

inline void Coord_sph2car(float &angy, float &angz, const float dis, Vertex &v)
{
	v.z = dis * sin(angy*PI / 180) * cos(angz*PI / 180.0);
	v.x = dis * sin(angy*PI / 180) * sin(angz*PI / 180);
	v.y = dis * cos(angy*PI / 180);
}

inline void Coord_sph2car2(float &angy, float &angz, const float dis, Vertex &v)
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

inline void Coord_car2sph(const Vertex &v, float &angy, float &angz, float &dis)
{
	dis = v.length();
	angy = acos(v.y / dis) * 180 / PI;
	angz = atan2(v.x, v.z) * 180 / PI;
}

float BorderTest(const Ray & ray, const Vertex &Min, const Vertex &Max)
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

	float dmin = dismin.x < dismin.y ? dismin.y : dismin.x,
		dmax = dismax.x < dismax.y ? dismax.x : dismax.y;
	dmin = dmin < dismin.z ? dismin.z : dmin,
		dmax = dmax < dismax.z ? dmax : dismax.z;
	dmin = dmin < 0 ? 0 : dmin;
	if (dmax < dmin)
		return 1e20;
	return dmin;
}



Coord2D Coord2D::operator+(const Coord2D &c) const
{
	return Coord2D(u + c.u, v + c.v);
}

Coord2D Coord2D::operator*(const float &n) const
{
	return Coord2D(u*n, v*n);
}



Vertex::Vertex()
{
#ifdef SSE2
	dat = _mm_setzero_ps();
#else
	x = y = z = 0;
#endif
}
Vertex::Vertex(const __m128 &idat)
{
	_mm_store_ps((float*)&dat, idat);
	//dat = idat;
}
float Vertex::length() const
{
	return sqrt(x*x + y*y + z*z);
}
float Vertex::length_sqr() const
{
#ifdef SSE2
	__m128 ans = _mm_dp_ps(dat, dat, 0b01110001);
	return ans.m128_f32[0];
#else
	return x*x + y*y + z*z;
#endif
}
Vertex Vertex::muladd(const float & n, const Vertex & v) const
{
	//__m256d tmp = _mm256_set1_pd(n);
	//return _mm256_fmadd_pd(dat, tmp, v.dat);
	return Vertex();
}
Vertex Vertex::mixmul(const Vertex & v) const
{
#ifdef SSE2
	return _mm_mul_ps(dat, v.dat);
#else
	return Vertex(x * v.x, y * v.y, z * v.z);
#endif
}
Vertex Vertex::operator+(const Vertex &v) const
{
#ifdef SSE2
	return _mm_add_ps(dat, v.dat);
#else
	return Vertex(x + v.x, y + v.y, z + v.z);
#endif
}
Vertex &Vertex::operator+=(const Vertex & right)
{
#ifdef SSE2
	dat = _mm_add_ps(dat, right.dat);
#else
	x += right.x, y += right.y, z += right.z;
#endif
	return *this;
}
Vertex Vertex::operator-(const Vertex &v) const
{
#ifdef SSE2
	return _mm_sub_ps(dat, v.dat);
#else
	return Vertex(x - v.x, y - v.y, z - v.z);
#endif
}
Vertex &Vertex::operator-=(const Vertex & right)
{
#ifdef SSE2
	dat = _mm_sub_ps(dat, right.dat);
#else
	x += right.x, y += right.y, z += right.z;
#endif
	return *this;
}
Vertex Vertex::operator/(const float &n) const
{
	float rec = 1 / n;
#ifdef SSE2
	__m128 tmp = _mm_set1_ps(rec);
	return _mm_mul_ps(dat, tmp);
#else
	return Vertex(x * rec, y * rec, z * rec);
	//return Vertex(x / n, y / n, z / n);
#endif
}
Vertex &Vertex::operator/=(const float & right)
{
	float rec = 1 / right;
#ifdef SSE2
	__m128 tmp = _mm_set1_ps(rec);
	dat = _mm_mul_ps(dat, tmp);
#else
	x *= rec, y *= rec, z *= rec;
#endif
	return *this;
}
Vertex Vertex::operator*(const float &n) const
{
#ifdef SSE2
	__m128 tmp = _mm_set1_ps(n);
	return _mm_mul_ps(dat, tmp);
#else
	return Vertex(x * n, y * n, z * n);
#endif
}
Vertex &Vertex::operator*=(const float & right)
{
#ifdef SSE2
	__m128 tmp = _mm_set1_ps(right);
	dat = _mm_mul_ps(dat, tmp);
#else
	x *= right, y *= right, z *= right;
#endif
	return *this;
}
Vertex Vertex::operator*(const Vertex &v) const
{
#ifdef SSE2
	//return _mm_dp_ps(dat, v.dat, 0x55);
#else

	__m256d t1 = _mm256_permute4x64_pd(dat, _MM_SHUFFLE(3, 0, 2, 1)),
		t4 = _mm256_permute4x64_pd(v.dat, _MM_SHUFFLE(3, 0, 2, 1)),
		t2 = _mm256_permute4x64_pd(v.dat, _MM_SHUFFLE(3, 1, 0, 2)),
		t3 = _mm256_permute4x64_pd(dat, _MM_SHUFFLE(3, 1, 0, 2));
	__m256d left = _mm256_mul_pd(t1, t2),
		right = _mm256_mul_pd(t3, t4);
	return _mm256_sub_pd(left, right);

#endif

	float a, b, c;
	a = y*v.z - z*v.y;
	b = z*v.x - x*v.z;
	c = x*v.y - y*v.x;
	return Vertex(a, b, c);
//#endif
}
float Vertex::operator&(const Vertex &v) const
{
#ifdef SSE4
	__m128 ans = _mm_dp_ps(dat, v.dat, 0b01110001);
	return ans.m128_f32[0];
#else
	return x*v.x + y*v.y + z*v.z;
#endif
}



Normal::Normal(const Vertex &v)//¹éÒ»»¯
{
	float s = v.x*v.x + v.y*v.y + v.z*v.z;
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

Texture::Texture(Texture && t)
{
	name = move(t.name);
	w = t.w, h = t.h;
	swap(data, t.data);
	if (t.data != nullptr)
	{
		delete[] t.data;
		t.data = nullptr;
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

void Material::SetMtl(int8_t prop, float r, float g, float b, float a)
{
	Vertex set(r, g, b, a);
	if (prop & MY_MODEL_AMBIENT)
		ambient = set;
	if (prop & MY_MODEL_DIFFUSE)
		diffuse = set;
	if (prop & MY_MODEL_SHINESS)
		shiness = set;
	if (prop & MY_MODEL_EMISSION)
		emission = set;
	if (prop & MY_MODEL_SPECULAR)
		specular = set;
}



Triangle::Triangle()
{
}

Triangle::Triangle(const Vertex &va, const Vertex &vb, const Vertex &vc)
{
	points[0] = va, points[1] = vb, points[2] = vc;
	norms[0] = Normal(), norms[1] = Normal(), norms[2] = Normal();
	tcoords[0] = Coord2D(), tcoords[1] = Coord2D(), tcoords[2] = Coord2D();
}

Triangle::Triangle(const Vertex &va, const Normal &na, const Vertex &vb, const Normal &nb, const Vertex &vc, const Normal &nc)
{
	points[0] = va, points[1] = vb, points[2] = vc;
	norms[0] = na, norms[1] = nb, norms[2] = nc;
	tcoords[0] = Coord2D(), tcoords[1] = Coord2D(), tcoords[2] = Coord2D();
}

Triangle::Triangle(const Vertex &va, const Normal &na, const Coord2D &ta, const Vertex &vb, const Normal &nb, const Coord2D &tb, const Vertex &vc, const Normal &nc, const Coord2D &tc)
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

Color::Color(const float depth, const float mindepth, const float maxdepth)
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
	float after = log(depth), max = log(maxdepth);
	r = g = b = (max - after) * 255 / max;
}

Color::Color(const Vertex &v)
{
	r = v.x > 255 ? 255 : (uint8_t)v.x;
	g = v.y > 255 ? 255 : (uint8_t)v.y;
	b = v.z > 255 ? 255 : (uint8_t)v.z;
}

Color::Color(const Normal &n)
{
	r = 127 * (n.x + 1);
	g = 127 * (n.y + 1);
	b = 127 * (n.z + 1);
}

Color::Color(const int16_t & w, const int16_t & h, const uint8_t *data, const Coord2D &coord)
{

	float empty,
		nu = modf(coord.u, &empty),
		nv = modf(coord.v, &empty);
	if (nu < 0)
		nu += 1;
	if (nv < 0)
		nv += 1;
	int16_t x = (int16_t)(nu * w),
		y = (int16_t)(nv * h);
	int32_t offset = (y * w + x) * 3;
	b = data[offset];
	g = data[offset + 1];
	r = data[offset + 2];
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



Sphere::Sphere(const float r, GLuint lnum) : DrawObject(lnum)
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

HitRes Sphere::intersect(const Ray &ray, const HitRes &hr, const float min)
{
	if (hr.distance < min)//early quit
		return hr;
	/*
	** s2r->vector that sphere's origin towards ray
	** t = -d.s2r-sqrt[(d.s2r)^2-(s2r^2-r^2)]
	*/
	Vertex s2r = ray.origin - position;
	if (abs(s2r.length_sqr() - radius_sqr) < 1e-5)//on it's self
		return hr;
	float rdDOTr2s = ray.direction & s2r;
	if (rdDOTr2s > 0)
		return hr;
	float dis = rdDOTr2s * rdDOTr2s - s2r.length_sqr() + radius_sqr;
	if (dis < 0)
		return hr;
	float t = -((ray.direction & s2r) + sqrt(dis));
	if (t < hr.distance && t > 1e-6)
	{
		HitRes newhr(t);
		newhr.position = ray.origin + ray.direction * t;
		newhr.normal = Normal(newhr.position - position);
		newhr.mtl = &mtl;
		return newhr;
	}
	return hr;
}



Box::Box(const float len, GLuint lnum) : DrawObject(lnum)
{
	type = MY_OBJECT_CUBE;
	width = height = length = len;
	float l = len / 2;
	max = Vertex(l, l, l);
	min = max * -1;
}

Box::Box(const float l, const float w, const float h, GLuint lnum) : DrawObject(lnum)
{
	type = MY_OBJECT_CUBE;
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
	glVertex3f(max.x, min.y, max.z);
	glVertex3f(max.x, max.y, max.z);
	glVertex3f(min.x, max.y, max.z);
	glVertex3f(min.x, min.y, max.z);
	//right
	glNormal3d(1, 0, 0);
	glVertex3f(max.x, min.y, max.z);
	glVertex3f(max.x, max.y, max.z);
	glVertex3f(max.x, max.y, min.z);
	glVertex3f(max.x, min.y, min.z);
	//back
	glNormal3d(0, 0, -1);
	glVertex3f(min.x, min.y, min.z);
	glVertex3f(min.x, max.y, min.z);
	glVertex3f(max.x, max.y, min.z);
	glVertex3f(max.x, min.y, min.z);
	//left
	glNormal3d(-1, 0, 0);
	glVertex3f(min.x, min.y, max.z);
	glVertex3f(min.x, max.y, max.z);
	glVertex3f(min.x, max.y, min.z);
	glVertex3f(min.x, min.y, min.z);
	//up
	glNormal3d(0, 1, 0);
	glVertex3f(max.x, max.y, max.z);
	glVertex3f(max.x, max.y, min.z);
	glVertex3f(min.x, max.y, min.z);
	glVertex3f(min.x, max.y, max.z);
	//down
	glNormal3d(0, -1, 0);
	glVertex3f(max.x, min.y, min.z);
	glVertex3f(max.x, min.y, max.z);
	glVertex3f(min.x, min.y, max.z);
	glVertex3f(min.x, min.y, min.z);

	glEnd();
	glEndList();
}

HitRes Box::intersect(const Ray & ray, const HitRes &hr, const float dmin)
{
	if (hr.distance < dmin)//early quit
		return hr;
	float res = BorderTest(ray, min + position, max + position);
	if (res < hr.distance && res > 1e-6)
	{
		HitRes newhr(res);
		newhr.position = ray.origin + ray.direction * res;
		Vertex b2p = newhr.position - position;
		Vertex point;
		if (abs(abs(b2p.z) - max.z) < 1e-6)//front or back
			point.z = b2p.z>0 ? 1 : -1;
		if (abs(abs(b2p.y) - max.y) < 1e-6)//up or down
			point.y = b2p.y>0 ? 1 : -1;
		if (abs(abs(b2p.x) - max.x) < 1e-6)//left or right
			point.x = b2p.x>0 ? 1 : -1;
		newhr.normal = Normal(point);
		newhr.mtl = &mtl;
		return newhr;
	}
	else
		return hr;
}



Plane::Plane(GLuint lnum) : DrawObject(lnum)
{
	type = MY_OBJECT_PLANE;
	rotate(Vertex(0, 36, 0));
}

void Plane::rotate(const Vertex & v)
{
	bool fix = false;
	ang.x = mod(360 + ang.x + v.y * 5, 360);
	ang.y = mod(360 + ang.y + v.x * 5, 360);
	ang.z += v.z;
	if (abs(ang.z) < 1e-5)
		fix = true, ang.z = 1;
	Coord_sph2car2(ang.x, ang.y, ang.z, position);
	normal = Normal(position * -1);
	if (fix)
		ang.z = 0, position = Vertex();
}

void Plane::GLPrepare()
{
	glNewList(GLListNum, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mtl.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mtl.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mtl.specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mtl.shiness);
	glMaterialfv(GL_FRONT, GL_EMISSION, mtl.emission);


	float tangy = mod(90 + ang.x, 360);
	Normal tmpy;
	Coord_sph2car2(tangy, ang.y, 1, tmpy);
	Normal tmpx = tmpy * normal;
	Vertex tmp;

	glBegin(GL_QUADS);
	glNormal3fv(normal);
	tmp = (tmpy * 5) + (tmpx * 5);
	glVertex3fv(tmp);
	tmp = (tmpy * 5) + (tmpx * -5);
	glVertex3fv(tmp);
	tmp = (tmpy * -5) + (tmpx * -5);
	glVertex3fv(tmp);
	tmp = (tmpy * -5) + (tmpx * 5);
	glVertex3fv(tmp);
	glEnd();
	glEndList();
}

HitRes Plane::intersect(const Ray & ray, const HitRes & hr, const float dmin)
{
	float a = ray.direction & normal;
	if (abs(a) < 1e-6)
		return hr;
	Vertex p2r = ray.origin - position;
	float b = p2r & normal;
	float dis = -b / a;
	if (dis < 0)
		return hr;
	if (dis < hr.distance)
	{
		HitRes newhr(dis);
		newhr.normal = normal;
		newhr.mtl = &mtl;
		//return hr;
		return newhr;
	}
	else
		return hr;
}



Light::Light(const uint8_t type)
{
	this->type = type;
	bLight = true;
	rangy = 90, rangz = 0, rdis = 16;
	move(0, 0, 0);
	SetProperty(MY_MODEL_AMBIENT, 0.05f, 0.05f, 0.05f);
	SetProperty(MY_MODEL_DIFFUSE | MY_MODEL_SPECULAR, 1.0f, 1.0f, 1.0f);
	SetProperty(MY_MODEL_ATTENUATION, 1.0f, 0.0f, 0.0f);
	switch (type)
	{
	case MY_MODEL_LIGHT_PARALLEL:
		position.alpha = 0.0f;
		break;
	case MY_MODEL_LIGHT_POINT:
		position.alpha = 1.0f;
		break;
	case MY_MODEL_LIGHT_SPOT:
		position.alpha = 1.0f;
		break;
	}
}

bool Light::turn()
{
	return bLight = !bLight;
}

void Light::move(const float dangy, const float dangz, const float ddis)
{
	rdis += ddis;
	if (rdis < 2)
		rdis = 2;
	else if (rdis > 64)
		rdis = 64;
	angy = rangy = mod(360 + rangy + dangy, 360);
	angz = rangz = mod(360 + rangz + dangz, 360);
	dis = rdis;

	Coord_sph2car2(angy, angz, dis, position);
}

void Light::SetProperty(int16_t prop, float r, float g, float b, float a)
{
	Vertex set(r, g, b, a);
	if (prop & MY_MODEL_AMBIENT)
		ambient = set;
	if (prop & MY_MODEL_DIFFUSE)
		diffuse = set;
	if (prop & MY_MODEL_SPECULAR)
		specular = set;
	if (prop & MY_MODEL_ATTENUATION)
		attenuation = set;
	if (prop & MY_MODEL_POSITION)
		position = set;
}

void Light::SetLumi(const float lum)
{
	float ext = lum / attenuation.alpha;
	attenuation.alpha = lum;
	ambient *= ext;
	diffuse *= ext;
	specular *= ext;
}



Camera::Camera(GLint w, GLint h)
{
	width = w, height = h;
	aspect = (float)w / h;
	fovy = 45.0, zNear = 1.0, zFar = 100.0;

	position = Vertex(0, 4, 15);
	u = Vertex(1, 0, 0);
	v = Vertex(0, 1, 0);
	n = Vertex(0, 0, -1);
}

void Camera::move(const float & x, const float & y, const float & z)
{
	position += u*x;
	position += v*y;
	position += n*z;
}

void Camera::yaw(const float angz)
{
	//rotate n(toward)
	float oangy = acos(n.y / 1) * 180 / PI,
	oangz = atan2(n.x, n.z) * 180 / PI;
	oangz -= angz;
	Coord_sph2car(oangy, oangz, 1, n);
	//rotate u(right)
	oangy = acos(u.y / 1) * 180 / PI;
	oangz = atan2(u.x, u.z) * 180 / PI;
	oangz -= angz;
	Coord_sph2car(oangy, oangz, 1, u);
}

void Camera::pitch(float angy)
{
	//rotate n(toward)
	float oangy = acos(n.y / 1) * 180 / PI,
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
	aspect = (float)w / h;
}


