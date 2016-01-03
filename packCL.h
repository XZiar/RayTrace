#pragma once
#include "rely.h"
#include "3DElement.h"

class Model;
void CL_get(cl_context &ctx, cl_command_queue &cqu, cl_program &pgm, cl_kernel knl);
class packCL
{
private:
	cl_context context;
	cl_command_queue command_queue;
	cl_program program;
	cl_kernel kernel;
	cl_int ret;

	vector<cl_mem> clm_parts;
	vector<cl_mem> clm_ress;
	vector<int> tri_cnt;
	cl_mem clm_rays[16];
public:
	packCL();
	~packCL();

	void init(const Model *mod);
	int16_t dowork(const int8_t pID, const clRay &ray, Vertex &coord);
};
