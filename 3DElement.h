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


class Vertex
{
public:
	GLdouble x, y, z;
	GLfloat fx, fy, fz;
	Vertex();
	Vertex(GLdouble ix, GLdouble iy, GLdouble iz);
	GLdouble length() const;
	GLdouble length_sqr() const;

	Vertex operator+(const Vertex &v);
	Vertex operator-(const Vertex &v);
	Vertex operator/(const double &n);
	Vertex operator*(const double &n);
	Vertex operator*(const Vertex &v);
	GLdouble operator&(const Vertex &v);//点积
	operator GLdouble*();
	operator GLfloat*();
};

class Normal : public Vertex
{
public:
	Normal() : Vertex() { };
	Normal(GLdouble ix, GLdouble iy, GLdouble iz) :Vertex(ix, iy, iz) { };
	Normal(Vertex v);//归一化
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
	Color(bool black);
	void put(uint8_t *addr);
	void get(uint8_t *addr);
};

class Ray
{
public:
	Vertex origin,
		direction;

};

class HitRes
{
public:
	bool isHit;
	double distance;
	Vertex position;
	Normal normal;

	HitRes(bool b = false) : isHit(b) { };
	HitRes(double dis) : distance(dis), isHit(true) { };
	operator bool();
};

class DrawObject
{
protected:
	GLuint GLListNum, texList[32];
	virtual void GLPrepare() = 0;
	virtual HitRes intersect(Ray &ray) = 0;
public:
	Vertex position;
	DrawObject(GLuint n) : GLListNum(n) { };
	void GLDraw();
};

class Sphere : public DrawObject
{
private:
	double radius, radius_sqr;
	Material mtl;
public:
	Sphere(double r = 1.0, GLuint lnum = 0);
	void SetMtl(const Material &mtl);
	virtual void GLPrepare() override;
	virtual HitRes intersect(Ray &ray) override;
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
	GLdouble rangy, rangz, rdis,
		angy, angz, dis,
		tangy, tangz, tdis,
		oangy, oangz, odis;
	Vertex position, poi, head;
	GLint width, height;
	GLdouble fovy, aspect, zNear, zFar;
	Camera(GLint w = 1120, GLint h = 630);
	void move(const int8_t &dangy, const int8_t &dangz, const int8_t &ddis);
	void move(GLdouble x, GLdouble y);//move poi
	void resize(GLint w, GLint h);
};


void Coord_sph2car(double &angy, double &angz, double &dis, Vertex &v);
void Coord_car2sph(const Vertex &v, double &angy, double &angz, double &dis);
void Coord_car2xaxis(const Vertex &v, Vertex &xaxis);
void Coord_car2yaxis(const Vertex &v, Vertex &yaxis);