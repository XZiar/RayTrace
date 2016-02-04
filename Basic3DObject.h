#pragma once
#include "rely.h"
#include "3DElement.h"

class Sphere : public DrawObject
{
private:
	float radius, radius_sqr;
	float *vertices, *normals, *texcoords;
	GLushort *indices;
public:
	Sphere(const float r = 1.0f, GLuint lnum = 0);
	~Sphere() override;
	virtual void GLPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr, const float min = 0) override;
};

class Box : public DrawObject
{
private:
	static const float cube_v[6 * 4 * 3];
	static const float cube_n[6 * 4 * 3];
	static const float cube_t[8 * 2];

	float width, height, length;
	Vertex min, max;
public:

	Box(const float len = 2.0, GLuint lnum = 0);
	Box(const float l, const float w, const float h, GLuint lnum = 0);
	virtual void GLPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr, const float dmin = 0) override;
};

class Plane : public DrawObject
{
private:
	Vertex ang;
	Normal axisx, axisy;
	Texture tex;
	GLuint texList;
public:
	Normal normal;

	Plane(GLuint lnum = 0);
	void rotate(const Vertex &v);
	void setTex(const Texture &tex) { this->tex = tex; };
	virtual void GLPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr, const float min = 0) override;
};

class BallPlane : public DrawObject
{
private:
	Vertex ang;
	Normal axisx, axisy;
	float radius, radius_sqr;
	float *vertices, *normals, *texcoords;
	GLushort *indices;
public:
	Normal normal;

	BallPlane(const float r = 0.3f, GLuint lnum = 0);
	~BallPlane() override;
	void rotate(const Vertex &v);
	virtual void GLPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr, const float min = 0) override;
};


void CreateSphere(const float radius, const unsigned int rings, const unsigned int sectors, float *vertices, float *normals, float *texcoords, GLushort *indices);

float BorderTest(const Ray &ray, const Vertex &Min, const Vertex &Max, float *getMax);