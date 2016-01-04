#pragma once
#include "rely.h"

#define MY_OBJECT_SPHERE 0x1
#define MY_OBJECT_CUBE   0x2
#define MY_OBJECT_MODEL  0x3
#define MY_OBJECT_PLANE  0x4

const char MY_OBJECT_NAME[][10] =
{ "ERROR","sphere","cube","model","plane" };

#define MY_LIGHT_PARALLEL 0x1
#define MY_LIGHT_POINT    0x2
#define MY_LIGHT_SPOT     0x3

const char MY_LIGHT_NAME[][10] =
{ "ERROR","parallel","point","spot" };

#define MY_MODEL_AMBIENT     0x1
#define MY_MODEL_DIFFUSE     0x2
#define MY_MODEL_SPECULAR    0x4
#define MY_MODEL_SHINESS     0x8
#define MY_MODEL_EMISSION    0x10
#define MY_MODEL_POSITION    0x100
#define MY_MODEL_ATTENUATION 0x200


class Coord2D
{
public:
	float u, v;
	Coord2D() { u = v = 0.0; };
	Coord2D(const float &iu, const float &iv) :u(iu), v(iv) { };

	Coord2D operator+(const Coord2D &c) const;
	Coord2D operator*(const float &n) const;
	operator float*() { return &u; };
};

_MM_ALIGN16 class Vertex
{
public:
	union
	{
		__m128 dat;
		struct
		{
			float x, y, z, alpha;
		};
	};
	Vertex();
	Vertex(const __m128 &idat);
	Vertex(const float ix, const float iy, const float iz, const float ia = 0) :x(ix), y(iy), z(iz), alpha(ia) { };
	operator float*() { return &x; };
	operator __m128() const { return dat; };

	float length() const;
	float length_sqr() const;
	Vertex muladd(const float &n, const Vertex &v) const;
	Vertex mixmul(const Vertex &v) const;

	Vertex operator+(const Vertex &v) const;
	Vertex &operator+=(const Vertex &right);
	Vertex operator-(const Vertex &v) const;
	Vertex &operator-=(const Vertex &right);
	Vertex operator/(const float &n) const;
	Vertex &operator/=(const float &right);
	Vertex operator*(const float &n) const;
	Vertex &operator*=(const float &right);
	Vertex operator*(const Vertex &v) const;
	float operator&(const Vertex &v) const;//点积
};

class Normal : public Vertex
{
public:
	Normal() : Vertex() { };
	Normal(const float &ix, const float &iy, const float &iz) :Vertex(ix, iy, iz) { };
	Normal(const Vertex &v);//归一化
};

class Texture
{
public:
	string name;
	int16_t w, h;
	uint8_t *data = nullptr;
	Texture(const string &iname, const int16_t iw, const int16_t ih, const uint8_t *img);
	~Texture();
	Texture(const Texture& t);
	Texture(Texture &&t);
};

class Material
{
public:
	string name;
	int16_t w, h;
	Vertex ambient,
		diffuse,
		specular,
		shiness,
		emission;
	Material();
	~Material();
	void SetMtl(int8_t prop, float r, float g, float b, float a = 1.0f);
};

struct clTri
{
	Vertex axisu, axisv, p0;
	clTri(const Vertex &u = Vertex(), const Vertex &v = Vertex(), const Vertex &p = Vertex()) :axisu(u), axisv(v), p0(p) { };
};
class Triangle
{
public:
	Vertex points[3];
	Normal norms[3];
	Coord2D tcoords[3];
	
	Triangle();
	Triangle(const Vertex &va, const Vertex &vb, const Vertex &vc);
	Triangle(const Vertex &va, const Normal &na, const Vertex &vb, const Normal &nb, const Vertex &vc, const Normal &nc);
	Triangle(const Vertex &va, const Normal &na, const Coord2D &ta, const Vertex &vb, const Normal &nb, const Coord2D &tb, const Vertex &vc, const Normal &nc, const Coord2D &tc);
};

class Color
{
public:
	uint8_t r, g, b;
	Color(const bool black);
	Color(const float depth, const float mindepth, const float maxdepth);
	Color(const Vertex &v);
	Color(const Normal &n);
	Color(const int16_t &w, const int16_t &h, const uint8_t *data, const Coord2D &coord);
	void put(uint8_t *addr);
	void get(uint8_t *addr);
};

class Ray
{
public:
	Vertex origin;
	Normal direction;
	uint8_t type = 0x0;

	Ray(const Vertex &o, const Normal &dir) : origin(o), direction(dir) { };
};

class HitRes
{
public:
	Vertex position;
	Normal normal;
	Coord2D tcoord;
	float distance;
	Material *mtl = nullptr;
	Texture *tex = nullptr;
	intptr_t obj = (intptr_t)this;

	HitRes(bool b = false);
	HitRes(float dis) : distance(dis){ };
	
	bool operator<(const HitRes &right);
	operator bool();
};

class DrawObject
{
protected:
	GLuint GLListNum, texList[30];
public:
	Vertex position;
	uint8_t type;
	bool bShow = true;

	DrawObject(GLuint n) : GLListNum(n) { };
	virtual ~DrawObject() { };
	void GLDraw();
	virtual void GLPrepare() = 0;
	virtual void RTPrepare() { };
	virtual HitRes intersect(const Ray &ray, const HitRes &hr, const float min = 0) = 0;
};

class Light
{
public:
	Vertex position,
		ambient,
		diffuse,
		specular,
		attenuation;
	float rangy, rangz, rdis,
		angy, angz, dis;
	uint8_t type;
	bool bLight;

	Light(const uint8_t type);
	bool turn();
	void move(const float dangy, const float dangz, const float ddis);
	void SetProperty(const int16_t prop, const float r, const float g, const float b, const float a = 1.0f);
	void SetLumi(const float lum);
};

class Camera
{
public:
	Normal u, v, n;//to right,up,toward
	Vertex position;
	GLint width, height;
	float fovy, aspect, zNear, zFar;
	Camera(GLint w = 1120, GLint h = 630);
	void move(const float &x, const float &y, const float &z);
	void yaw(const float angz);
	void pitch(float angy);
	void resize(GLint w, GLint h);
};


void Coord_sph2car(float &angy, float &angz, const float dis, Vertex &v);
void Coord_sph2car2(float &angy, float &angz, const float dis, Vertex &v);
void Coord_car2sph(const Vertex &v, float &angy, float &angz, float &dis);
float mod(const float &l, const float &r);



