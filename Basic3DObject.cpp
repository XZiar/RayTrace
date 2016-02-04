#include "rely.h"
#include "Basic3DObject.h"

void CreateSphere(const float radius, const unsigned int rings, const unsigned int sectors, float *vertices, float *normals, float *texcoords, GLushort *indices)
{
	float const R = 1.0 / (float)(rings - 1);
	float const S = 1.0 / (float)(sectors - 1);

	float* v = vertices;
	float* n = normals;
	float* t = texcoords;
	GLushort *i = indices;

	for (int r = 0; r < rings; r++) 
		for (int s = 0; s < sectors; s++)
		{
			float const y = sin(-M_PI_2 + M_PI * r * R);
			float const x = cos(2 * M_PI * s * S) * sin(M_PI * r * R);
			float const z = sin(2 * M_PI * s * S) * sin(M_PI * r * R);
			*t++ = s*S;
			*t++ = r*R;
			*v++ = x * radius;
			*v++ = y * radius;
			*v++ = z * radius;

			*n++ = x;
			*n++ = y;
			*n++ = z;
		}

	for (int r = 0; r < rings - 1; r++)
		for (int s = 0; s < sectors - 1; s++)
		{
			*i++ = r * sectors + s;
			*i++ = r * sectors + (s + 1);
			*i++ = (r + 1) * sectors + (s + 1);
			*i++ = (r + 1) * sectors + s;
		}
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

	vertices = new float[80 * 80 * 3];
	normals = new float[80 * 80 * 3];
	texcoords = new float[80 * 80 * 2];
	indices = new GLushort[80 * 80 * 4];
	CreateSphere(0.5f, 80, 80, vertices, normals, texcoords, indices);
}

Sphere::~Sphere()
{
	delete[] vertices;
	delete[] normals;
	delete[] texcoords;
	delete[] indices;
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

	//glutSolidSphere(radius, 100, 100);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glNormalPointer(GL_FLOAT, 0, normals);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glEnable(GL_RESCALE_NORMAL);
	const float sr = radius * 2;
	glScalef(sr, sr, sr);
	glDrawElements(GL_QUADS, 79 * 79 * 4, GL_UNSIGNED_SHORT, indices);
	glDisable(GL_RESCALE_NORMAL);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glEndList();
}

HitRes Sphere::intersect(const Ray &ray, const HitRes &hr, const float min)
{
	//early quit
	if (hr.obj == (intptr_t)this)
	{
		if (!ray.isInside)
			return hr;
		//InSide
		if (ray.type == MY_RAY_SHADOWRAY)
			return HitRes(radius);
		Vertex s2r = ray.origin - position;
		float rdDOTr2s = ray.direction & s2r;
		float dis = rdDOTr2s * rdDOTr2s - s2r.length_sqr() + radius_sqr;
		float t = -(ray.direction & s2r) + sqrt(dis);
		if (t < hr.distance && t > 1e-6)
		{
			HitRes newhr(t);
			newhr.position = ray.origin + ray.direction * t;
			newhr.normal = Normal(position - newhr.position);
			newhr.mtl = &mtl;
			newhr.obj = (intptr_t)this;
			newhr.isInside = ~ray.isInside;
			if (ray.type == MY_RAY_REFRACTRAY)
				newhr.rfr = 1.0f;//leave sphere,set back rfr
			else
				newhr.rfr = mtl.rfr;//still in sphere
			return newhr;
		}
		else
			return hr;
	}
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
		newhr.isInside = ~ray.isInside;
		newhr.rfr = mtl.rfr;
		return newhr;
	}
	return hr;
}


/* Vertex Coordinates */
const float Box::cube_v[6 * 4 * 3] =
{
	.5, .5, .5,  -.5, .5, .5,  -.5,-.5, .5,  .5,-.5, .5,       // v0-v1-v2-v3 (front)

	.5, .5, .5,   .5,-.5, .5,   .5,-.5,-.5,   .5, .5,-.5,      // v0-v3-v4-v5 (right)

	.5, .5, .5,   .5, .5,-.5,  -.5, .5,-.5,  -.5, .5, .5,      // v0-v5-v6-v1 (top)

	-.5, .5, .5,  -.5, .5,-.5,  -.5,-.5,-.5,  -.5,-.5, .5,      // v1-v6-v7-v2 (left)

	-.5,-.5,-.5,   .5,-.5,-.5,   .5,-.5, .5,  -.5,-.5, .5,      // v7-v4-v3-v2 (bottom)

	.5,-.5,-.5,  -.5,-.5,-.5,  -.5, .5,-.5,   .5, .5,-.5,      // v4-v7-v6-v5 (back)
};
/* Normal Vectors */
const float Box::cube_n[6 * 4 * 3] =
{
	0, 0, 1,   0, 0, 1,   0, 0, 1,   0, 0, 1,      // v0-v1-v2 (front)

	1, 0, 0,   1, 0, 0,   1, 0, 0,   1, 0, 0,      // v0-v3-v4 (right)

	0, 1, 0,   0, 1, 0,   0, 1, 0,   0, 1, 0,      // v0-v5-v6 (top)

	-1, 0, 0,  -1, 0, 0,  -1, 0, 0,  -1, 0, 0,      // v1-v6-v7 (left)

	0,-1, 0,   0,-1, 0,   0,-1, 0,   0,-1, 0,      // v7-v4-v3 (bottom)

	0, 0,-1,   0, 0,-1,   0, 0,-1,   0, 0,-1,      // v4-v7-v6 (back)
};
/* Texture Coordinates */

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

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	//glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_RESCALE_NORMAL);
	Vertex tmp = max - min;
	glScalef(tmp.x, tmp.y, tmp.z);

	//glutSolidCube(1);
	glVertexPointer(3, GL_FLOAT, 0, cube_v);
	glNormalPointer(GL_FLOAT, 0, cube_n);
	//glTexCoordPointer(2, GL_FLOAT, 0, cube_t);
	glDrawArrays(GL_QUADS, 0, 6 * 4);
	//glDrawElements(GL_QUADS, 6 * 4, GL_UNSIGNED_SHORT, cube_vi);

	glDisable(GL_RESCALE_NORMAL);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	//glDisableClientState(GL_TEXTURE_COORD_ARRAY);

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



BallPlane::BallPlane(const float r, GLuint lnum) : DrawObject(lnum)
{
	type = MY_OBJECT_BALLPLANE;
	radius = r;
	radius_sqr = r * r;
	rotate(Vertex(0, 36, 0));

	vertices = new float[80 * 80 * 3];
	normals = new float[80 * 80 * 3];
	texcoords = new float[80 * 80 * 2];
	indices = new GLushort[80 * 80 * 4];
	CreateSphere(r, 80, 80, vertices, normals, texcoords, indices);
}

BallPlane::~BallPlane()
{
	delete[] vertices;
	delete[] normals;
	delete[] texcoords;
	delete[] indices;
}

void BallPlane::rotate(const Vertex & v)
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

void BallPlane::GLPrepare()
{
	glNewList(GLListNum, GL_COMPILE);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mtl.ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mtl.diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mtl.specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mtl.emission);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mtl.shiness);
	glBindTexture(GL_TEXTURE_2D, 0);

	Vertex tmp;

	for (auto cx = radius * -6; cx < radius * 8; cx += radius*4)
		for (auto cy = radius * -6; cy < radius * 8; cy += radius * 4)
		{
			tmp = axisx * cx + axisy * cy;
			glPushMatrix();
			glTranslatef(tmp.x, tmp.y, tmp.z);

			//glutSolidSphere(radius, 100, 100);
			glVertexPointer(3, GL_FLOAT, 0, vertices);
			glNormalPointer(GL_FLOAT, 0, normals);
			glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
			glDrawElements(GL_QUADS, 80 * 80 * 4, GL_UNSIGNED_SHORT, indices);

			glPopMatrix();
		}
	glEndList();
}

HitRes BallPlane::intersect(const Ray & ray, const HitRes & hr, const float min)
{
	//series of intersect of sphere
	Vertex tmp;
	int cnt = 0;
	HitRes newhr = hr;
	for (auto cx = radius * -6; cx < radius * 8; cx += radius * 4)
	for (auto cy = radius * -6; cy < radius * 8; cy += radius * 4)
	{
		++cnt;
		tmp = axisx * cx + axisy * cy;
		Vertex tposition = tmp + position;
		//early quit
		if (hr.obj == (intptr_t)this + cnt)
		{
			if (!ray.isInside)
				continue;
			//InSide
			if (ray.type == MY_RAY_SHADOWRAY)
				return HitRes(radius);
			Vertex s2r = ray.origin - tposition;
			float rdDOTr2s = ray.direction & s2r;
			float dis = rdDOTr2s * rdDOTr2s - s2r.length_sqr() + radius_sqr;
			float t = -(ray.direction & s2r) + sqrt(dis);
			if (t < newhr.distance && t > 1e-6)
			{
				newhr = HitRes(t);
				newhr.position = ray.origin + ray.direction * t;
				newhr.normal = Normal(tposition - newhr.position);
				newhr.mtl = &mtl;
				newhr.obj = (intptr_t)this + cnt;
				newhr.isInside = ~ray.isInside;
				if (ray.type == MY_RAY_REFRACTRAY)
					newhr.rfr = 1.0f;//leave sphere,set back rfr
				else
					newhr.rfr = mtl.rfr;//still in sphere
				continue;
			}
			else
				continue;
		}
		Vertex s2r = ray.origin - tposition;
		float rdDOTr2s = ray.direction & s2r;
		if (rdDOTr2s > 0)
			continue;
		float dis = rdDOTr2s * rdDOTr2s - s2r.length_sqr() + radius_sqr;
		if (dis < 0)
			continue;
		float t = -((ray.direction & s2r) + sqrt(dis));
		if (t < newhr.distance && t > 1e-6)
		{
			newhr = HitRes(t);
			newhr.position = ray.origin + ray.direction * t;
			newhr.normal = Normal(newhr.position - tposition);
			newhr.mtl = &mtl;
			newhr.obj = (intptr_t)this + cnt;
			newhr.isInside = ~ray.isInside;
			newhr.rfr = mtl.rfr;
			continue;
		}
		continue;
	}
	if (newhr.distance < hr.distance)
		return newhr;
	else
		return hr;
}