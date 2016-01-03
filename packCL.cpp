#include "rely.h"
#include "packCL.h"
#include "Model.h"
#pragma warning( disable : 4996 )


void CL_get(cl_context &ctx, cl_command_queue &cqu, cl_program &pgm, cl_kernel knl)
{
	static bool isFirst = true;
	static cl_platform_id platform_id = 0;
	static cl_uint ret_num_platforms;
	static cl_uint ret_num_devices;
	static cl_device_id device_id;
	static cl_context context;
	static cl_command_queue command_queue;
	static cl_program program;
	static cl_kernel kernel;

	cl_int ret;
	if (isFirst)
	{
		FILE *fp;
		char fileName[] = "D:\\Programs Data\\VSProject\\RayTrace\\RayTrace\\TriangleTest.cl";
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

		cl_uint numPlatforms = 0;
		clGetPlatformIDs(0, NULL, &numPlatforms);

		//获得平台列表  
		cl_platform_id *platforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));
		clGetPlatformIDs(numPlatforms, platforms, NULL);

		//轮询各个opencl设备  
		for (cl_uint i = 0; i < numPlatforms; i++)
		{
			char pBuf[2][100];
			clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(pBuf), pBuf[0], NULL);
			clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION, sizeof(pBuf), pBuf[1], NULL);
			if (IDYES == MessageBoxA(NULL, pBuf[1], pBuf[0], MB_ICONQUESTION | MB_YESNO))
			{
				platform_id = platforms[i];
				break;
			}
		}


		/* Get Platform and Device Info */
		//ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
		ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

		/* Create OpenCL context */
		context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

		/* Create Command Queue */
		command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

		/* Create Kernel Program from the source */
		program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);

		/* Build Kernel Program */
		ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
		if (ret != CL_SUCCESS)
		{
			size_t len;
			char buffer[204800];

			printf("Error: Failed to build program executable!\n");
			clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
			printf("%s\n", buffer);
			MessageBoxA(NULL, buffer, "Failed to build program executable!", MB_ICONSTOP);
			exit(1);
		}
		

		delete[] source_str;
		isFirst = false;
	}
	ctx = context;
	cqu = command_queue;
	pgm = program;
	knl = kernel;
	return;
}

packCL::packCL()
{
	CL_get(context, command_queue, program, kernel);
	/* Create OpenCL Kernel */
	kernel = clCreateKernel(program, "clTriTest", &ret);
	for (auto a = 0; a < 16; ++a)
	{
		clm_rays[a] = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(clRay), NULL, &ret);
	}
}

packCL::~packCL()
{
	/* Finalization */
	/*ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(memobj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);*/
}

void packCL::init(const Model *mod)
{
	for(auto &mo : clm_parts)
		clReleaseMemObject(mo);
	for (auto &mo : clm_ress)
		clReleaseMemObject(mo);
	clm_parts.clear();
	clm_ress.clear();
	tri_cnt.clear();
	
	for (auto &part : mod->clparts)
	{
		tri_cnt.push_back(part.size());
		cl_mem mo1 = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, part.size()*sizeof(clTri), (void*)&part[0], &ret);
		cl_mem mo2 = clCreateBuffer(context, CL_MEM_WRITE_ONLY, part.size() * 16, NULL, &ret);
		clm_parts.push_back(mo1);
		clm_ress.push_back(mo2);
	}
	for (auto a = 0; a < 16; ++a)
		clm_rays[a] = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(clRay), NULL, &ret);
}

int16_t packCL::dowork(const int8_t pID, const clRay &ray, Vertex &coord)
{
	int size = tri_cnt[pID];
	//put ray data
	
	ret = clEnqueueWriteBuffer(command_queue, clm_rays[0], TRUE, 0, 32, &ray, 0, NULL, NULL);
	
	//ret = clEnqueueWriteBuffer(command_queue, mray, TRUE, 0, 32, &ray, 0, NULL, NULL);

	/* Set OpenCL Kernel Parameters */
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&clm_rays[0]);
	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&clm_parts[pID]);
	ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&clm_ress[pID]);

	/* Execute OpenCL Kernel */
	size_t global_work_size[1] = { size };
	cl_event enentPoint;
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, &enentPoint);
	clWaitForEvents(1, &enentPoint); //wait
	clReleaseEvent(enentPoint);
	
	/* Copy results from the memory buffer */
	Vertex *fres = new Vertex[size];
	ret = clEnqueueReadBuffer(command_queue, clm_ress[pID], CL_TRUE, 0, 16 * size, fres, 0, NULL, &enentPoint);
	clWaitForEvents(1, &enentPoint); //wait
	clReleaseEvent(enentPoint);

	float fans = 1e10f;
	int choose = -1;

  	for (int a = 0; a < size; ++a)
	{
		if (fres[a].alpha < fans)
			fans = fres[a].alpha, choose = a;
	}
	if (fans < 1e10f)
		coord = fres[choose];
	else
		coord = Vertex(0, 0, 0, 1e20f);

	/*clean resources*/
	//ret = clReleaseMemObject(mray);
	delete[] fres;

	return choose;
}


