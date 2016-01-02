#include "rely.h"
#include "packCL.h"

packCL::packCL()
{
	FILE *fp;
	char fileName[] = "./TriangleTest.cl";
	char *source_str;
	size_t source_size;

	/* Load the source code containing the kernel*/
	fp = fopen(fileName, "r");
	if (!fp)
	{
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = new char[2048];
	source_size = fread(source_str, 1, 2048, fp);
	fclose(fp);

	/* Get Platform and Device Info */
	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

	/* Create OpenCL context */
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

	/* Create Command Queue */
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

	/* Create Kernel Program from the source */
	program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);

	/* Build Kernel Program */
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	/* Create OpenCL Kernel */
	kernel = clCreateKernel(program, "triTest", &ret);

	delete[] source_str;

}

packCL::~packCL()
{
	/* Finalization */
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(memobj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);
}

void packCL::init(const int8_t tNum, const Model &mod)
{
	for(auto &mo : modeldat)
		clReleaseMemObject(mo);
	for (auto &mo : raydat)
		clReleaseMemObject(mo);
	modeldat.clear();
	for (auto &part : mod.newparts)
	{
		cl_mem mo = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, part.size()*sizeof(Triangle), &part[0], &ret);
		modeldat.push_back(mo);
	}
	for (auto a = 0; a < tNum; ++a)
	{
		cl_mem mo = clCreateBuffer(context, CL_MEM_READ_ONLY, 32, NULL, &ret);
		raydat.push_back(mo);
	}
}

float packCL::dowork(const int8_t tID, const Ray &ray)
{
	cl_event wrt;
	//put ray data
	ret = clEnqueueWriteBuffer(command_queue, raydat[tID], TRUE, 0, 32, &ray, 0, NULL, &wrt);




	/* Set OpenCL Kernel Parameters */
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&memobj);

	/* Execute OpenCL Kernel */
	ret = clEnqueueTask(command_queue, kernel, 0, NULL, NULL);

	/* Copy results from the memory buffer */
	ret = clEnqueueReadBuffer(command_queue, memobj, CL_TRUE, 0,
		MEM_SIZE * sizeof(char), string, 0, NULL, NULL);
}
