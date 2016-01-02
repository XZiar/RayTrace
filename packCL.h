#pragma once
#include "rely.h"
#include "Model.h"

class packCL
{
private:
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
	cl_mem memobj = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_platform_id platform_id = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret;
	vector<cl_mem> modeldat;
	vector<cl_mem> raydat;
public:
	packCL();
	~packCL();

	void init(const int8_t tNum, const Model &mod);
	float dowork(const int8_t tID, const Ray &ray);
};
