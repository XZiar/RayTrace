#include "rely.h"
#include "3DElement.h"
#include "Basic3DObject.h"
#include "Model.h"
#include "Scene.h"
#include "RayTracer.h"

static bool bMovPOI = false;
static int sx, sy, mx, my;
static Scene scene;
static RayTracer rayt(scene);
static volatile uint8_t obj_toggle = 0xff, lgt_toggle = 0xff;
static Camera &cam = scene.cam;
static wstring filename[2];
static volatile bool Mode = true, isRun = true;

void onMenu(int val);
void BaseTest(bool isAuto = false);


static void ChgMode(const bool b)
{
	Mode = b;
	glutSetWindowTitle(Mode ? "OpenGL" : "RayTracer");
}
int8_t FileDlg(wstring &filename, wstring &mtlname)
{
	OPENFILENAME ofn;      // 公共对话框结构。     
	wchar_t szFile[MAX_PATH]; // 保存获取文件名称的缓冲区。               

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"OBJ Model(*.obj)\0*.obj\0OBJ Material(*.mtl)\0*.mtl\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&ofn))
	{
		wstring fname(szFile);
		if (fname.substr(fname.size() - 3, fname.size() - 1) == L"mtl")
			mtlname = fname;
		else
		{
			filename = fname;
			mtlname = fname.substr(0, fname.size() - 3) + L"mtl";
		}
		return 1;
	}
	else
		return -1;
}

void InitMenu()
{
	int num = glutGet(GLUT_MENU_NUM_ITEMS);
	for (auto a = 0; a < num;++a)
		glutRemoveMenuItem(a);

	vector<int> menuID;
	
	for (auto a = 0; a < scene.Lights.size(); ++a)
	{
		int base = 0x200 + (a << 4);
		int ID = glutCreateMenu(onMenu);
		char label[32];
		sprintf(label, "===%s===", MY_LIGHT_NAME[scene.Lights[a].type]);
		glutAddMenuEntry(label, 0x0);
		glutAddMenuEntry("Toggle", base + 0x0);
		glutAddMenuEntry("Enable/Disable", base + 0x1);
		glutAddMenuEntry("Delete", base + 0x2);
		menuID.push_back(ID);
	}
	for (auto a = 0; a < scene.Objects.size(); ++a)
	{
		int base = 0x100 + (a << 4);
		int ID = glutCreateMenu(onMenu);
		char label[32];
		sprintf(label, "===%s===", MY_OBJECT_NAME[scene.Objects[a]->type]);
		glutAddMenuEntry(label, 0x0);
		glutAddMenuEntry("Toggle", base + 0x0);
		glutAddMenuEntry("Enable/Disable", base + 0x1);
		glutAddMenuEntry("Delete", base + 0x2);
		if(dynamic_cast<Model*>(scene.Objects[a]) != NULL)
			glutAddMenuEntry("Z-axis Rotate", base + 0x3);
		menuID.push_back(ID);
	}
	//Add Light Menu
	int ID = glutCreateMenu(onMenu);
	glutAddMenuEntry("Parallel Light", 0x011);
	glutAddMenuEntry("Point Light", 0x012);
	glutAddMenuEntry("Spot Light", 0x013);

	glutCreateMenu(onMenu);
	glutAddSubMenu("Add Light", ID);
	glutAddMenuEntry("Add Sphere", 0x001);
	glutAddMenuEntry("Add Cube", 0x002);
	glutAddMenuEntry("Add Model", 0x003);
	glutAddMenuEntry("Add Plane", 0x004);
	glutAddMenuEntry("---Lights---", 0x0);
	int a = 0;
	for (; a < scene.Lights.size(); ++a)
	{
		char label[32];
		sprintf(label, "Light %d", a);
		glutAddSubMenu(label, menuID[a]);
	}
	glutAddMenuEntry("---Objects---", 0x0);
	for (auto b = 0; a < menuID.size(); ++a, ++b)
	{
		char label[32];
		sprintf(label, "Object %2d", b);
		glutAddSubMenu(label, menuID[a]);
	}
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void init(void)
{
	//basic setting
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
	
	//set depth test
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	//set texture environment
	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//set light environment
	glEnable(GL_LIGHTING);
	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

	//set light
	scene.EnvLight = Vertex(0.05f, 0.05f, 0.05f, 1.0f);
	lgt_toggle = scene.AddLight(MY_LIGHT_PARALLEL, Vertex(0.1f, 0.45f, 0.45f));
	scene.MovePos(MY_MODEL_LIGHT, lgt_toggle, Vertex(-45, 45, 0));
	lgt_toggle = scene.AddLight(MY_LIGHT_POINT, Vertex(0.15f, 0.55f, 0.3f), Vertex(0.0f, 0.0f, 1.0f, 256));
	/*scene.Lights[0] = Light();
	Light *lit = &scene.Lights[0];
	lit->SetProperty(MY_MODEL_SPECULAR, 0.5f, 0.5f, 0.5f);
	lit->SetProperty(MY_MODEL_DIFFUSE, 0.5f, 0.5f, 0.5f);
	lit->SetProperty(MY_MODEL_POSITION, 10.0f, 10.0f, 5.0f, 0.0f);
	scene.Lights[1] = Light(MY_MODEL_LIGHT_POINT);
	lit = &scene.Lights[1];
	lit->SetProperty(MY_MODEL_SPECULAR, 0.6f, 0.6f, 0.6f);
	lit->SetProperty(MY_MODEL_DIFFUSE, 0.4f, 0.4f, 0.4f);
	lit->SetProperty(MY_MODEL_ATTENUATION, 0.0f, 0.0f, 1.0f);
	lit->SetLumi(256);*/

	//init scene
	scene.init();
	//add ground
	scene.AddPlane();
	//add basic sphere
	obj_toggle = scene.AddSphere(1.0);

}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glLoadIdentity();

	if (Mode)//openGL Mode
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(cam.fovy, cam.aspect, cam.zNear, cam.zFar);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		//draw scene
		scene.DrawScene();
		//draw light
		scene.DrawLight(1);
	}
	else//RayTrace Mode
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0, cam.width, 0.0, cam.height);
		//gluOrtho2D(0, 3.2, 0, 1.8);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		
		glRasterPos2s(0, 0);
		glDrawPixels(rayt.width, rayt.height, GL_RGB, GL_UNSIGNED_BYTE, rayt.output);
		
		/*
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBindTexture(GL_TEXTURE_2D, rayt.texID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rayt.width, rayt.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, rayt.output);
		//glDisable(GL_TEXTURE_2D);
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2d(0, 0);//左下
		glTexCoord2f(0.0f, 1.0f); glVertex2d(0, 1);//左上
		glTexCoord2f(1.0f, 1.0f); glVertex2d(1, 1);//右上
		glTexCoord2f(1.0f, 0.0f); glVertex2d(1, 0);//右下
		glEnd();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		*/
	}
	glutSwapBuffers();
}

void reshape(int w, int h)
{
	mx = w, my = h;
	cam.resize(w & 0x8fc0, h & 0x8fc0);
	glViewport((w & 0x3f) / 2, (h & 0x3f) / 2, cam.width, cam.height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(cam.fovy, cam.aspect, cam.zNear, cam.zFar);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void onTimer(int value)
{
	if (!Mode)
	{
		glutPostRedisplay();
	}
	if(!rayt.isFinish)
		glutTimerFunc(50, onTimer, 1);
}

void onKeyboard(int key, int x, int y)
{
	if (!rayt.isFinish || !Mode)
		return;
	switch (key)
	{
	case GLUT_KEY_LEFT:
		cam.yaw(-3);break;
	case GLUT_KEY_RIGHT:
		cam.yaw(3);break;
	case GLUT_KEY_UP:
		cam.pitch(3);break;
	case GLUT_KEY_DOWN:
		cam.pitch(-3);break;
	case GLUT_KEY_END:
		BaseTest();break;
	case GLUT_KEY_F1:
	case GLUT_KEY_F2:
	case GLUT_KEY_F3:
	case GLUT_KEY_F4:
	case GLUT_KEY_F5:
	case GLUT_KEY_F6:
	case GLUT_KEY_F7:
	case GLUT_KEY_F8:
		scene.Switch(MY_MODEL_LIGHT | MY_MODEL_SWITCH, key - GLUT_KEY_F1, true);
		break;
	default:
		return;
	}
	glutPostRedisplay();
}

void onKeyboard(unsigned char key, int x, int y)
{
	if (key == 13)//enter
	{
		ChgMode(!Mode);
		glutPostRedisplay();
		return;
	}
	if (key == 32)//scape
	{
		ChgMode(false);
		if (rayt.isFinish)
		{
			rayt.start(MY_MODEL_RAYTRACE, 4);
			glutTimerFunc(50, onTimer, 1);
		}
		else
			rayt.stop();

		glutPostRedisplay();
		return;
	}
	if (glutGetModifiers() == GLUT_ACTIVE_ALT && key >= '0' && key <= '9')
	{
		ChgMode(false);
		//ray trace render
		if (!rayt.isFinish)
			rayt.stop();
		//start ray-trace
		else
		{
			int tnum = 8;
			switch (key)
			{
			case '1':
				rayt.start(MY_MODEL_CHECK, tnum);break;
			case '2':
				rayt.start(MY_MODEL_DEPTHTEST, tnum);break;
			case '3':
				rayt.start(MY_MODEL_NORMALTEST, tnum);break;
			case '4':
				rayt.start(MY_MODEL_TEXTURETEST, tnum);break;
			case '5':
				rayt.start(MY_MODEL_MATERIALTEST, tnum);break;
			case '6':
				rayt.start(MY_MODEL_SHADOWTEST, tnum); break;
			}
			glutTimerFunc(50, onTimer, 1);
		}
		glutPostRedisplay();
		return;
	}
	if (!rayt.isFinish || !Mode)
		return;
	switch (key)
	{
	//move camera
	case 'h':
		cam.move(1, 0, 0);break;
	case 'f':
		cam.move(-1, 0, 0);break;
	case 't':
		cam.move(0, 1, 0);break;
	case 'g':
		cam.move(0, -1, 0);break;
	case 27:
		exit(0);
		return;
	//move light
	case 'q'://light near
		scene.MovePos(MY_MODEL_LIGHT, lgt_toggle, Vertex(0, 0, -1)); break;
	case 'e'://light far
		scene.MovePos(MY_MODEL_LIGHT, lgt_toggle, Vertex(0, 0, 1)); break;
	case 'a':
		scene.MovePos(MY_MODEL_LIGHT, lgt_toggle, Vertex(0, -3, 0)); break;
	case 'd':
		scene.MovePos(MY_MODEL_LIGHT, lgt_toggle, Vertex(0, 3, 0)); break;
	case 'w':
		scene.MovePos(MY_MODEL_LIGHT, lgt_toggle, Vertex(-3, 0, 0)); break;
	case 's':
		scene.MovePos(MY_MODEL_LIGHT, lgt_toggle, Vertex(3, 0, 0)); break;
	//set light component
	case 'z':
		scene.ChgLightComp(MY_MODEL_SWITCH, lgt_toggle, Vertex(1.5, 1, 1)); break;
	case 'Z':
		scene.ChgLightComp(MY_MODEL_SWITCH, lgt_toggle, Vertex(0.67, 1, 1)); break;
	case 'x':
		scene.ChgLightComp(MY_MODEL_SWITCH, lgt_toggle, Vertex(1, 1.5, 1)); break;
	case 'X':
		scene.ChgLightComp(MY_MODEL_SWITCH, lgt_toggle, Vertex(1, 0.67, 1)); break;
	case 'c':
		scene.ChgLightComp(MY_MODEL_SWITCH, lgt_toggle, Vertex(1, 1, 1.5)); break;
	case 'C':
		scene.ChgLightComp(MY_MODEL_SWITCH, lgt_toggle, Vertex(1, 1, 0.67)); break;
	case 'v':
		scene.ChgLightComp(MY_MODEL_SWITCH, lgt_toggle, Vertex(1, 1, 1, 2)); break;
	case 'V':
		scene.ChgLightComp(MY_MODEL_SWITCH, lgt_toggle, Vertex(1, 1, 1, 0.5)); break;
	//move object
	case '2':
	case '4':
	case '6':
	case '8':
	case 43://+
	case 45://-
		switch (key)
		{
		case '2':
			scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { 0,-1,0 }); break;
		case '4':
			scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { -1,0,0 }); break;
		case '6':
			scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { 1,0,0 }); break;
		case '8':
			scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { 0,1,0 }); break;
		case 43://+
			scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { 0,0,1 }); break;
		case 45://-
			scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { 0,0,-1 }); break;
		}
		break;
	default:
		return;
	}
	glutPostRedisplay();
}

void onMouse(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN)
		{
			bMovPOI = true;
			sx = x, sy = y;
		}
		else
			bMovPOI = false;
		return;
	}
	
}

void onWheel(int button, int dir, int x, int y)
{
	if (dir == 1)//forward
		cam.move(0, 0, -1);
	else if(dir == -1)//backward
		cam.move(0, 0, 1);
	glutPostRedisplay();

}

void onMouse(int x, int y)
{
	if (bMovPOI)
	{
		int dx = x - sx, dy = y - sy;
		sx = x, sy = y;
		float pdx = 10.0*dx / cam.width, pdy = 10.0*dy / cam.height;
		cam.move(-pdx, pdy, 0);
		//cam.move(pdx, pdy);
		glutPostRedisplay();
	}
}

void BaseTest(bool isAuto)
{
	scene.MovePos(MY_MODEL_LIGHT, lgt_toggle, Vertex(-42, -57, 1));
	{
		filename[0] = L"F:\\Project\\RayTrace\\objs\\0.obj";
		filename[1] = L"F:\\Project\\RayTrace\\objs\\0.mtl";
		obj_toggle = scene.AddModel(filename[0], filename[1]);
		Model &model = dynamic_cast<Model&>(*scene.Objects[obj_toggle]);
		model.zRotate();
		scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { -9,0,-2 });
	}
	{
		filename[0] = L"F:\\Project\\RayTrace\\objs\\1.obj";
		filename[1] = L"F:\\Project\\RayTrace\\objs\\1.mtl";
		obj_toggle = scene.AddModel(filename[0], filename[1]);
		Model &model = dynamic_cast<Model&>(*scene.Objects[obj_toggle]);
		model.zRotate();
		scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { -2,0,4 });
	}
	{
		obj_toggle = scene.AddCube(1.0);
		scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { -2,2,10 });
	}
	{
		filename[0] = L"F:\\Project\\RayTrace\\objs\\2.obj";
		filename[1] = L"F:\\Project\\RayTrace\\objs\\2.mtl";
		obj_toggle = scene.AddModel(filename[0], filename[1]);
		Model &model = dynamic_cast<Model&>(*scene.Objects[obj_toggle]);
		model.zRotate();
		scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { 3,0,0 });
	}
	{
		filename[0] = L"F:\\Project\\RayTrace\\objs\\3.obj";
		filename[1] = L"F:\\Project\\RayTrace\\objs\\3.mtl";
		obj_toggle = scene.AddModel(filename[0], filename[1]);
		Model &model = dynamic_cast<Model&>(*scene.Objects[obj_toggle]);
		model.zRotate();
		scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { 7,0,3 });
	}
	{
		obj_toggle = scene.AddCube(1.0);
		scene.MovePos(MY_MODEL_OBJECT, obj_toggle, { 2.5,5,2 });
	}
	InitMenu();
	if (isAuto)
	{
		ChgMode(false);
		rayt.start(MY_MODEL_SHADOWTEST, 8);
		glutTimerFunc(50, onTimer, 1);
		thread([] 
		{ 
			Sleep(5000);
			while (!rayt.isFinish) { Sleep(1000); }
			exit(0);
		}).detach();
	}
}

void onMenu(int val)
{
	uint8_t obj = (val & 0xf0) >> 4;
	switch (val & 0xf00)
	{
	case 0x000://system
		switch (val)
		{
		case 0x01://Add Sphere
			obj_toggle = scene.AddSphere(1.0f);
			InitMenu();
			break;
		case 0x02://Add Cube
			obj_toggle = scene.AddCube(1.6f);
			InitMenu();
			break;
		case 0x03://Add Model
			if (FileDlg(filename[0], filename[1]) == -1)
				return;
			obj_toggle = scene.AddModel(filename[0], filename[1]);
			InitMenu();
			break;
		case 0x04:
			obj_toggle = scene.AddPlane();
			InitMenu();
			break;
		case 0x11://Parallel Light
			lgt_toggle = scene.AddLight(MY_LIGHT_PARALLEL, Vertex(0.1f, 0.45f, 0.45f));
			break;
		case 0x12://Point Light
			lgt_toggle = scene.AddLight(MY_LIGHT_POINT, Vertex(0.15f, 0.55f, 0.3f), Vertex(0.0f, 0.0f, 1.0f, 256));
			break;
		case 0x13://Spot Light
			break;
		default:
			return;
		}
		break;
	case 0x100://objects
		switch (val & 0xf)
		{
		default:
			return;
		case 0://toggle
			obj_toggle = obj;
			return;
		case 1://Enable/Disable
			scene.Switch(MY_MODEL_OBJECT | MY_MODEL_SWITCH, obj, true);
			break;
		case 2://Delete
			scene.Delete(MY_MODEL_OBJECT, obj);
			obj_toggle = 0xff;
			InitMenu();
			break;
		case 3://z-rotate
			Model &model = dynamic_cast<Model&>(*scene.Objects[obj]);
			model.zRotate();
			break;
		}
		break;
	case 0x200://lights
		switch (val & 0xf)
		{
		default:
			return;
		case 0://toggle
			lgt_toggle = obj;
			return;
		case 1://Enable/Disable
			scene.Switch(MY_MODEL_LIGHT | MY_MODEL_SWITCH, obj, true);
			break;
		case 2://Delete
			scene.Delete(MY_MODEL_LIGHT, obj);
			lgt_toggle = 0xff;
			InitMenu();
			break;
		}
		break;
	}
	glutPostRedisplay();
}

void showdata()
{
	wprintf(L"Triangle size=%zd\tHitRes size=%zd\n", sizeof(Triangle), sizeof(HitRes));
	HANDLE hOut;
	COORD pos = { 0,1 };
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	std::locale::global(std::locale(""));
	while (isRun)
	{
		SetConsoleCursorPosition(hOut, pos);
		wprintf(L"方向键移动摄像机，wasd键移动灯，+-号缩放，12键开关灯\n");

		wprintf(L"相机绝对坐标：\t%4f，%4f，%4f\n", cam.position.x, cam.position.y, cam.position.z);
		wprintf(L"相机u向量坐标：\t%4f，%4f，%4f\n", cam.u.x, cam.u.y, cam.u.z);
		wprintf(L"相机v向量坐标：\t%4f，%4f，%4f\n", cam.v.x, cam.v.y, cam.v.z);
		wprintf(L"相机n向量坐标：\t%4f，%4f，%4f\n", cam.n.x, cam.n.y, cam.n.z);
		if (lgt_toggle != 0xff)
		{
			Light &light = scene.Lights[lgt_toggle];
			wprintf(L"%d号灯球坐标：\t%4f，%4f，%4f\n", lgt_toggle, light.angy, light.angz, light.dis);
			wprintf(L"%d号灯参数：\t%4f，%4f，%4f，%4f\n", lgt_toggle, light.ambient.x, light.diffuse.x, light.specular.x, light.attenuation.alpha);
		}
		else
			wprintf(L"目前未选中任何灯\n无该灯光的详细信息");
		if (obj_toggle != 0xff)
		{
			Vertex &pos = scene.Objects[obj_toggle]->position;
			wprintf(L"%d号物体坐标：\t%4f，%4f，%4f\n", obj_toggle, pos.x, pos.y, pos.z);
		}
		else
			wprintf(L"目前未选中任何物体\n");
		if (rayt.isFinish)
			wprintf(L"Finish in %4f s\n", rayt.useTime);
		else
			wprintf(L"Running... ...\t\t\n");
		Sleep(33);
	}
	return;
}


int wmain(int argc, wchar_t *argv[])
{
	char* tv[] = { "good" };
	int tc = 1;
	//_CrtSetBreakAlloc(1322268);
	SetCurrentDirectory(L"F:\\Project\\RayTrace\\objs");
	glutInit(&tc, tv);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(cam.width, cam.height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("");
	init();
	InitMenu();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(onKeyboard);
	glutSpecialFunc(onKeyboard);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMouse);
	glutMouseWheelFunc(onWheel);
	glutCloseFunc([] 
	{ 
		isRun = false;
		rayt.stop();
		while (!rayt.isFinish)
			Sleep(10);
	});

	thread(showdata).detach();
	if (argc > 1)
		thread([] { Sleep(100); BaseTest(true); }).detach();
	glutMainLoop();

	//_CrtDumpMemoryLeaks();
	return 0;
}
