#include "rely.h"
#include "3DElement.h"

inline float mod(const float &l, const float &r)
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
	dat = idat;
}
float Vertex::length() const
{
#ifdef SSE4
	__m128 ans = _mm_dp_ps(dat, dat, 0b01110001);
	return sqrt(_mm_cvtss_f32(ans));
#else
	return sqrt(x*x + y*y + z*z);
#endif
}
float Vertex::length_sqr() const
{
#ifdef SSE4
	__m128 ans = _mm_dp_ps(dat, dat, 0b01110001);
	return _mm_cvtss_f32(ans);
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
	return _mm_mul_ps(dat, v);
#else
	return Vertex(x * v.x, y * v.y, z * v.z);
#endif
}
Vertex Vertex::operator+(const Vertex &v) const
{
#ifdef SSE2
	return _mm_add_ps(dat, v);
#else
	return Vertex(x + v.x, y + v.y, z + v.z);
#endif
}
Vertex &Vertex::operator+=(const Vertex & right)
{
#ifdef SSE2
	return *this = _mm_add_ps(dat, right);
#else
	x += right.x, y += right.y, z += right.z;
	return *this;
#endif
}
Vertex Vertex::operator-(const Vertex &v) const
{
#ifdef SSE2
	return _mm_sub_ps(dat, v);
#else
	return Vertex(x - v.x, y - v.y, z - v.z);
#endif
}
Vertex &Vertex::operator-=(const Vertex & right)
{
#ifdef SSE2
	return *this = _mm_sub_ps(dat, right);
#else
	x += right.x, y += right.y, z += right.z;
	return *this;
#endif
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
	return *this = _mm_mul_ps(dat, tmp);
#else
	x *= rec, y *= rec, z *= rec;
	return *this;
#endif
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
	return *this = _mm_mul_ps(dat, tmp);
#else
	x *= right, y *= right, z *= right;
	return *this;
#endif
}
Vertex Vertex::operator*(const Vertex &v) const
{
#ifdef FMA
	__m128 t1 = _mm_permute_ps(dat, _MM_SHUFFLE(3, 0, 2, 1)),
		t2 = _mm_permute_ps(v.dat, _MM_SHUFFLE(3, 1, 0, 2)),
		t3 = _mm_permute_ps(dat, _MM_SHUFFLE(3, 1, 0, 2)),
		t4 = _mm_permute_ps(v.dat, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 r = _mm_mul_ps(t3, t4);
	return _mm_fmsub_ps(t1, t2, r);
#else
  #ifdef AVX
	__m128 t1 = _mm_permute_ps(dat, _MM_SHUFFLE(3, 0, 2, 1)),
		t2 = _mm_permute_ps(v.dat, _MM_SHUFFLE(3, 1, 0, 2)),
		t3 = _mm_permute_ps(dat, _MM_SHUFFLE(3, 1, 0, 2)),
		t4 = _mm_permute_ps(v.dat, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 l = _mm_mul_ps(t1, t2),
		r = _mm_mul_ps(t3, t4);
	return _mm_sub_ps(l, r);
  #else
	float a, b, c;
	a = y*v.z - z*v.y;
	b = z*v.x - x*v.z;
	c = x*v.y - y*v.x;
	return Vertex(a, b, c);
  #endif
#endif
}
float Vertex::operator&(const Vertex &v) const
{
#ifdef SSE4
	__m128 ans = _mm_dp_ps(dat, v.dat, 0b01110001);
	return _mm_cvtss_f32(ans);
#else
	return x*v.x + y*v.y + z*v.z;
#endif
}



Normal::Normal(const Vertex &v)//¹éÒ»»¯
{
#ifdef AVX2
	__m128 ans = _mm_dp_ps(v.dat, v.dat, 0b01110001);
	__m128 tmp = _mm_broadcastss_ps(_mm_sqrt_ss(ans));
	dat = _mm_div_ps(v.dat, tmp);
#else
  #ifdef SSE4
	__m128 ans = _mm_dp_ps(v.dat, v.dat, 0b01110001);
	ans = _mm_sqrt_ss(ans);
	__m128 tmp = _mm_set1_ps(_mm_cvtss_f32(ans));
	dat = _mm_div_ps(v.dat, tmp);
  #else
	float s = v.x*v.x + v.y*v.y + v.z*v.z;
	s = 1 / sqrt(s);
	x = v.x * s;
	y = v.y * s;
	z = v.z * s;
  #endif
#endif
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
	distance = b ? 1e8 : 1e20;
}

bool HitRes::operator<(const HitRes & right)
{
	return distance < right.distance;
}

HitRes::operator bool()
{
	return distance < 1e8;
}



void DrawObject::GLDraw()
{
	glCallList(GLListNum);
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
	case MY_LIGHT_PARALLEL:
		position.alpha = 0.0f;
		break;
	case MY_LIGHT_POINT:
		position.alpha = 1.0f;
		break;
	case MY_LIGHT_SPOT:
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


