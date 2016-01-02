#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define USING_FREEGLUT
#define SSE2
#ifdef SSE2
#define SSE4
#ifdef SSE4
#define AVX
#ifdef AVX
#define FMA
#define AVX2
#endif
#endif
#endif
#define USING_OPENCL
#ifndef _DEBUG
#define NDEBUG
#endif

#include <GL/glew.h>
#pragma comment(lib,"glew32.lib")
#ifdef USING_FREEGLUT
#define FREEGLUT_STATIC
#include <GL/freeglut.h>//Free GLUT Header
#else
#include <GL/glut.h>   // The GL Utility Toolkit (Glut) Header
#endif

#define PI 3.1415926535897932384
#define _MM_ALIGN32 _VCRT_ALIGN(32)

#include <intrin.h>
#include <cstdio>
#include <conio.h>
#include <cstdlib>
#include <cstdint>
#include <locale>
#include <cmath>
#include <Windows.h>
#include <commdlg.h>
#include <vector>
#include <algorithm>
#include <tuple>
#include <memory>
#include <thread>
#include <atomic>

#ifdef USING_OPENCL
  #include <CL\opencl.h>
  #pragma comment(lib, "opencl.lib")
#endif



using namespace std;
using namespace std::placeholders;