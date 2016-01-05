#include "rely.h"
#include "Basic3DObject.h"

float BorderTestOld(const Ray & ray, const Vertex &Min, const Vertex &Max)
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

float BorderTest(const Ray & ray, const Vertex &Min, const Vertex &Max, float *getMax)
{
	Vertex tdismin = Min - ray.origin, tdismax = Max - ray.origin;
	Vertex rrd = _mm_div_ps(_mm_set1_ps(1.0f), ray.direction);
	tdismin = tdismin.mixmul(rrd);
	tdismax = tdismax.mixmul(rrd);
	Vertex dismin = _mm_min_ps(tdismin.dat, tdismax.dat),
		dismax = _mm_max_ps(tdismin.dat, tdismax.dat);
	
	//test y
	if (abs(ray.direction.y) < 1e-6)
	{
		if (ray.origin.y > Max.y || ray.origin.y < Min.y)
			return 1e20f;
		dismin.y = -1, dismax.y = 1e10f;
	}
	//test x
	if (abs(ray.direction.x) < 1e-6)
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

void Sphere::GLPrepare()
{
	glNewList(GLListNum, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mtl.ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mtl.diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mtl.specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mtl.emission);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mtl.shiness);
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

void Box::GLPrepare()
{
	glNewList(GLListNum, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mtl.ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mtl.diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mtl.specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mtl.emission);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mtl.shiness);

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
	tex = Texture(true);
	glGenTextures(1, &texList);
	rotate(Vertex(0, 36, 0));
}

void Plane::rotate(const Vertex & v)
{
	bool fix = false;
	ang.x = mod(360 + ang.x - v.y * 5, 360);
	ang.y = mod(360 + ang.y - v.x * 5, 360);
	ang.z += v.z;
	if (ang.z < 0.0f)
		ang.z = 0.0f;
	if (abs(ang.z) < 1e-5f)
		fix = true, ang.z = 1;
	Coord_sph2car2(ang.x, ang.y, ang.z, position);
	normal = Normal(position * -1);
	if (fix)
		ang.z = 0, position = Vertex();

	float tangy = mod(90 + ang.x, 360);
	Coord_sph2car2(tangy, ang.y, 1, axisy);
	axisx = axisy * normal;
}

void Plane::GLPrepare()
{
	glBindTexture(GL_TEXTURE_2D, texList);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.w, tex.h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, tex.data);

	glNewList(GLListNum, GL_COMPILE);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mtl.ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mtl.diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mtl.specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mtl.emission);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mtl.shiness);
	glBindTexture(GL_TEXTURE_2D, texList);
	
	float range = 500;
	Vertex tmpx = axisx * range, tmpy = axisy * range;
	range /= 5;
	Vertex tmp;
	glBegin(GL_QUADS);
	glNormal3fv(normal);
	tmp = tmpy + tmpx;
	glTexCoord2f(range, range); glVertex3fv(tmp);
	tmp = tmpy + tmpx * -1;
	glTexCoord2f(-range, range); glVertex3fv(tmp);
	tmp = tmpy * -1 + tmpx * -1;
	glTexCoord2f(-range, -range); glVertex3fv(tmp);
	tmp = tmpy * -1 + tmpx;
	glTexCoord2f(range, -range); glVertex3fv(tmp);
	glEnd();
	/*glBegin(GL_TRIANGLES);
	tmp = axisy;
	glVertex3fv(tmp);
	tmp = axisx + axisy;
	glVertex3fv(tmp);
	tmp = normal + axisy;
	glVertex3fv(tmp);
	glEnd();*/
	glBindTexture(GL_TEXTURE_2D, 0);
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
	{//calculate coord for texture
		Vertex tmp1 = ray.direction * axisy;
		float f = 1.0f / (axisx & tmp1) / 5;
		float u = (p2r & tmp1) * f;
		Vertex tmp2 = p2r * axisx;
		float v = (ray.direction & tmp2) * f;

		HitRes newhr(dis);
		newhr.normal = normal;
		newhr.mtl = &mtl;
		newhr.tex = &tex;
		newhr.position = ray.origin + ray.direction*dis;
		float ppdis = (newhr.position - position).length_sqr();
		newhr.tcoord = Coord2D(u, v);
		newhr.obj = (intptr_t)this;
		return newhr;
	}
	else
		return hr;
}
