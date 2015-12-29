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
	Scene *scene;
	volatile bool isRun = false;
	thread t[32];
	bool state[32];
	double costtime[32];
	atomic_int16_t aBlock_Cur;

	void RTcheck(int8_t tNum, int8_t tID);
	void RTdepth(int8_t tNum, int8_t tID);
	void RTnorm(int8_t tNum, int8_t tID);
	void RTtex(int8_t tNum, int8_t tID);
	void RTthread(int8_t tNum, int8_t tID);
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

