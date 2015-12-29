#pragma once
#include "rely.h"

#define MY_MODEL_LIGHT_PARALLEL 0x1
#define MY_MODEL_LIGHT_POINT 0x2
#define MY_MODEL_LIGHT_SPOT 0x4

#define MY_MODEL_AMBIENT 0x1
#define MY_MODEL_DIFFUSE 0x2
#define MY_MODEL_SHINESS 0x4
#define MY_MODEL_EMISSION 0x8
#define MY_MODEL_SPECULAR 0x10
#define MY_MODEL_POSITION 0x100
#define MY_MODEL_ATTENUATION 0x200


class Coord2D
{
	GLdouble u, v;
	Coord2D();
	Coord2D(const GLdouble iu, const GLdouble iv);
};

_MM_ALIGN32 class Vertex
{
public:
	union
	{
		__m256d dat;
		struct
		{
			GLdouble x, y, z, alpha;
		};
	};
	Vertex();
	Vertex(const __m256d &idat);
	Vertex(const GLdouble &ix, const GLdouble &iy, const GLdouble &iz, const GLdouble &ia = 0.0);
	GLdouble length() const;
	GLdouble length_sqr() const;

	Vertex operator+(const Vertex &v) const;
	Vertex &operator+=(const Vertex &right);
	Vertex operator-(const Vertex &v) const;
	Vertex &operator-=(const Vertex &right);
	Vertex operator/(const double &n) const;
	Vertex &operator/=(const double &right);
	Vertex operator*(const double &n) const;
	Vertex operator*(const Vertex &v) const;
	GLdouble operator&(const Vertex &v) const;//点积
	operator GLdouble*();
};

class Normal : public Vertex
{
public:
	Normal() : Vertex() { };
	Normal(const GLdouble ix, const GLdouble iy, const GLdouble iz) :Vertex(ix, iy, iz) { };
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
};

class Material
{
public:
	string name;
	int16_t w, h;
	GLfloat ambient[4],
		diffuse[4],
		specular[4],
		shiness[4],
		emission[4];
	Material();
	~Material();
	void SetMtl(int8_t prop, GLfloat r, GLfloat g, GLfloat b, GLfloat a = 1.0f);
};

class Triangle
{
public:
	Vertex points[3];
	Normal norms[3];
	Vertex tcoords[3];
	
	Triangle();
	Triangle(Vertex va, Vertex vb, Vertex vc);
	Triangle(Vertex va, Normal na, Vertex vb, Normal nb, Vertex vc, Normal nc);
	Triangle(Vertex va, Normal na, Vertex ta, Vertex vb, Normal nb, Vertex tb, Vertex vc, Normal nc, Vertex tc);
};

class Color
{
public:
	uint8_t r, g, b;
	Color(const bool black);
	Color(const double depth, const double mindepth, const double maxdepth);
	Color(const Normal n);
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
	double distance;
	Vertex position;
	Normal normal;

	HitRes(bool b = false);
	HitRes(double dis) : distance(dis){ };
	
	bool operator<(const HitRes &right);
	operator bool();
};

class DrawObject
{
protected:
	GLuint GLListNum, texList[30];
	virtual void GLPrepare() = 0;
public:
	Vertex position;

	DrawObject(GLuint n) : GLListNum(n) { };
	virtual ~DrawObject() { };
	void GLDraw();
	virtual void RTPrepare() { };
	virtual HitRes intersect(const Ray &ray, const HitRes &hr) = 0;
};

class Sphere : public DrawObject
{
private:
	double radius, radius_sqr;
	Material mtl;
public:
	Sphere(const double r = 1.0, GLuint lnum = 0);

	void SetMtl(const Material &mtl);
	virtual void GLPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr) override;
};

class Box : public DrawObject
{
private:
	double width, height, length;
	Vertex min, max;
	Material mtl;
public:
	Box(const double len = 2.0, GLuint lnum = 0);
	Box(const double l, const double w, const double h, GLuint lnum = 0);
	void SetMtl(const Material &mtl);
	virtual void GLPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr) override;
};

class Light
{
public:
	bool bLight;
	GLdouble rangy, rangz, rdis,
		angy, angz, dis;
	GLfloat position[4],
		ambient[4],
		diffuse[4],
		specular[4];
	GLfloat attenuation[3];
	Light(int8_t type = 0x0);
	bool turn();
	void move(const int8_t &dangy, const int8_t &dangz, const int8_t &ddis);
	void SetProp(int16_t prop, GLfloat r, GLfloat g, GLfloat b, GLfloat a = 1.0f);
};

class Camera
{
public:
	Normal u, v, n;//to right,up,toward
	Vertex position;
	GLint width, height;
	GLdouble fovy, aspect, zNear, zFar;
	Camera(GLint w = 1120, GLint h = 630);
	void move(const double &x, const double &y, const double &z);
	void yaw(const double angz);
	void pitch(double angy);
	void resize(GLint w, GLint h);
};


void Coord_sph2car(double &angy, double &angz, const double dis, Vertex &v);
void Coord_car2sph(const Vertex &v, double &angy, double &angz, double &dis);
double BorderTest(const Ray & ray, const Vertex &Min, const Vertex &Max);


