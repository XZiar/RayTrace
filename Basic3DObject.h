#pragma once
#include "rely.h"
#include "3DElement.h"

class Sphere : public DrawObject
{
private:
	float radius, radius_sqr;
public:
	Material mtl;

	Sphere(const float r = 1.0, GLuint lnum = 0);
	void SetMtl(const Material &mtl);
	virtual void GLPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr, const float min = 0) override;
};

class Box : public DrawObject
{
private:
	float width, height, length;
	Vertex min, max;
public:
	Material mtl;

	Box(const float len = 2.0, GLuint lnum = 0);
	Box(const float l, const float w, const float h, GLuint lnum = 0);
	void SetMtl(const Material &mtl);
	virtual void GLPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr, const float dmin = 0) override;
};

class Plane : public DrawObject
{
private:
	Vertex ang;
public:
	Material mtl;
	Normal normal;

	Plane(GLuint lnum = 0);
	void rotate(const Vertex &v);
	virtual void GLPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr, const float dmin = 0) override;
};

