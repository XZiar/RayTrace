#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define USING_FREEGLUT true

#ifndef _DEBUG
#define NDEBUG
#endif

#include <GL/glew.h>
#pragma comment(lib,"glew32.lib")
#if USING_FREEGLUT
#define FREEGLUT_STATIC
#include <GL/freeglut.h>//Free GLUT Header
#else
#include <GL/glut.h>   // The GL Utility Toolkit (Glut) Header
#endif

#define PI 3.1415926535897932384

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
using namespace std;