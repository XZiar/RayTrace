#pragma once

#include "Scene.h"

#define MY_MODEL_CHECK 0x1
#define MY_MODEL_DEPTHTEST 0x2
#define MY_MODEL_NORMALTEST 0x3
#define MY_MODEL_TEXTURETEST 0x4
#define MY_MODEL_RAYTRACE 0x80

class RayTracer
{
private:
	using PR = function<Color(const double, const double, const Ray&)>;
	Scene *scene;
	volatile bool isRun = false;
	thread t[32];
	bool state[32];
	volatile double costtime[32];
	atomic_int16_t aBlock_Cur;

	void parallelRT(const int8_t tNum, const int8_t tID, const PR &worker);
	void RTcheck(const int8_t tNum, const int8_t tID);
	Color RTdepth(const double zNear, const double zFar, const Ray &baseray);
	Color RTnorm(const double zNear, const double zFar, const Ray &baseray);
	Color RTtex(const double zNear, const double zFar, const Ray &baseray);
public:
	GLuint texID;
	uint8_t *output;
	volatile bool isFinish = true;
	volatile double useTime;
	int width, height;
	RayTracer(Scene &scene);
	~RayTracer();

	void start(const uint8_t type, const int8_t tnum = 1);
	void stop();
};

