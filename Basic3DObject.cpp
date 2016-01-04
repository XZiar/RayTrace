#include "rely.h"
#include "Basic3DObject.h"


float BorderTest(const Ray & ray, const Vertex &Min, const Vertex &Max, float *getMax)
{
	Vertex tdismin = Min - ray.origin, tdismax = Max - ray.origin;
	Vertex rrd = _mm_div_ps(_mm_set1_ps(1.0f), ray.direction);
	tdismin = tdismin.mixmul(rrd);
	tdismax = tdismax.mixmul(rrd);
	Vertex dismin = _mm_min_ps(tdismin.dat, tdismax.dat),
		dismax = _mm_max_ps(tdismin.dat, tdismax.dat);
	
	//test y
	if (abs(ray.direction.z) < 1e-6)
	{
		if (ray.origin.y > Max.y || ray.origin.y < Min.y)
			return 1e20f;
		dismin.y = -1, dismax.y = 1e10f;
	}
	//test x
	if (abs(ray.direction.z) < 1e-6)
	{
		if (ray.origin.x > Max.x || ray.origin.x < Min.x)
			return 1e20f;
		dismin.x = -1, dismax.x = 1e10f;
	}
	//test z
	if (abs(ray.direction.z) < 1e-6)
	{
		if (ray.origin.z > Max.z || ray.origin.z < Min.z)
			return 1e20f;
		dismin.z = -1, dismax.z = 1e10f;
	}

	float dmin = max(max(dismin.x, dismin.y), max(dismin.z, 0.0f)),
		dmax = min(min(dismax.x, dismax.y), dismax.z);
	if (dmax < dmin)
		return 1e20;
	*getMax = dmax;
	return dmin;
}



Sphere::Sphere(const float r, GLuint lnum) : DrawObject(lnum)
{
	type = MY_OBJECT_SPHERE;
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
	//early quit
	if (hr.obj == (intptr_t)this)
		return hr;
	/*
	** s2r->vector that sphere's origin towards ray
	** t = -d.s2r-sqrt[(d.s2r)^2-(s2r^2-r^2)]
	*/
	Vertex s2r = ray.origin - position;
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
		newhr.obj = (intptr_t)this;
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

	glEnable(GL_RESCALE_NORMAL);
	Vertex tmp = max - min;
	glScalef(tmp.x, tmp.y, tmp.z);
	glutSolidCube(1);
	glDisable(GL_RESCALE_NORMAL);
	/*
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
	*/
	glEndList();
}

HitRes Box::intersect(const Ray & ray, const HitRes &hr, const float dmin)
{
	//early quit
	if (hr.obj == (intptr_t)this)
		return hr;
	float empty;
	float res = BorderTest(ray, min + position, max + position, &empty);
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
		newhr.obj = (intptr_t)this;
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
	Vertex tmpy;
	Coord_sph2car2(tangy, ang.y, 1, tmpy);
	Vertex tmpx = tmpy * normal;
	float range = 500;
	tmpx *= range, tmpy *= range;
	Vertex tmp;
	glBegin(GL_QUADS);
	glNormal3fv(normal);
	tmp = tmpy + tmpx;
	glVertex3fv(tmp);
	tmp = tmpy + tmpx * -1;
	glVertex3fv(tmp);
	tmp = tmpy * -1 + tmpx * -1;
	glVertex3fv(tmp);
	tmp = tmpy * -1 + tmpx;
	glVertex3fv(tmp);
	glEnd();
	glEndList();
}

HitRes Plane::intersect(const Ray & ray, const HitRes & hr, const float min)
{
	//early quit
	if (hr.obj == (intptr_t)this)
		return hr;

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
		newhr.position = ray.origin + ray.direction*dis;
		newhr.obj = (intptr_t)this;
		return newhr;
	}
	else
		return hr;
}
