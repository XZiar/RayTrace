#pragma once

#include "Scene.h"

#define MY_MODEL_CHECK 0x1
#define MY_MODEL_DEPTHTEST 0x2
#define MY_MODEL_NORMALTEST 0x3
#define MY_MODEL_TEXTURETEST 0x4
#define MY_MODEL_MATERIALTEST 0x5
#define MY_MODEL_SHADOWTEST 0x6
#define MY_MODEL_REFLECTTEST 0x7
#define MY_MODEL_REFRACTTEST 0x8
#define MY_MODEL_RAYTRACE 0x80


class RayTracer
{
private:
	//using PR = function<Color(const float, const float, const Ray&)>;
	Scene *scene;
	volatile bool isRun = false;
	thread t[32];
	bool state[32];
	volatile double costtime[32];
	atomic_int16_t aBlock_Cur;
	HitRes tmphr; bool isPrt = false;

	typedef Color(RayTracer::*PerRay)(float, float, const Ray&);

	void parallelRT(const int8_t tNum, const int8_t tID, const PerRay &worker);
	void RTcheck(const int8_t tNum, const int8_t tID);
	Color RTdepth(const float zNear, const float zFar, const Ray &baseray);
	Color RTnorm(const float zNear, const float zFar, const Ray &baseray);
	Color RTtex(const float zNear, const float zFar, const Ray &baseray);
	Color RTmtl(const float zNear, const float zFar, const Ray &baseray);
	Color RTshd(const float zNear, const float zFar, const Ray &baseray);
	Color RTflec(const float zNear, const float zFar, const Ray &baseray,
		const uint8_t level, const float bwc, HitRes &hr);//loop count , benefit weight , base hitres
	Color RTfrac(const float zNear, const float zFar, const Ray &baseray,
		const uint8_t level, const float bwc, HitRes &hr);//loop count , benefit weight , base hitres

	Color proxyRTflec(const float zNear, const float zFar, const Ray &baseray);
	Color proxyRTfrac(const float zNear, const float zFar, const Ray &baseray);
public:
	GLuint texID;
	uint8_t *output;
	volatile bool isFinish = true;
	volatile double useTime;
	uint8_t maxLevel = 1;
	int width, height;
	RayTracer(Scene &scene);
	~RayTracer();

	void start(const uint8_t type, const int8_t tnum = 1);
	void stop();
};

