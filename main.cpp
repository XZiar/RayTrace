#include "rely.h"
#include "3DElement.h"
#include "Model.h"
#include "Scene.h"
#include "RayTracer.h"

static bool bMovPOI = false;
static int sx, sy, mx, my;
static Scene scene;
static RayTracer rayt(scene);
static int16_t obj_toggle = -1;
static Light &light = scene.Lights[1];
static Camera &cam = scene.cam;
static GLuint dList[8];
static wstring filename[2];
static int menu, entry[4];
static uint8_t code = 0x0;

static bool Mode = true;

void onMenu(int val);
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
	for (auto a = 0; a < scene.Objects.size(); ++a)
	{
		int base = (a + 1) << 4;
		int ID = glutCreateMenu(onMenu);
		glutAddMenuEntry("Toggle", base + 0x0);
		glutAddMenuEntry("Enable/Disable", base + 0x1);
		glutAddMenuEntry("Delete", base + 0x2);
		if(dynamic_cast<Model*>(get<0>(scene.Objects[a])) != NULL)
			glutAddMenuEntry("Z-axis Rotate", base + 0x3);
		menuID.push_back(ID);
	}
	glutCreateMenu(onMenu);
	glutAddMenuEntry("Add Sphere", 0x0);
	glutAddMenuEntry("Add Cube", 0x1);
	glutAddMenuEntry("Add Model", 0x2);
	for (auto a = 0; a < menuID.size(); ++a)
	{
		char label[50];
		sprintf(label, "Object %d", a);
		glutAddSubMenu(label, menuID[a]);
	}
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void init(void)
{
	glEnable(GL_DEPTH_TEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//glEnable(GL_NORMALIZE);
	glClearColor(0.1f, 0.1f, 0.1f, 0.0);
	glShadeModel(GL_SMOOTH);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	scene.Lights[0] = Light(MY_MODEL_LIGHT_PARALLEL);
	Light *lit = &scene.Lights[0];
	lit->SetProp(MY_MODEL_SPECULAR, 0.3f, 0.3f, 0.3f);
	lit->SetProp(MY_MODEL_DIFFUSE, 0.15f, 0.15f, 0.15f);
	lit->SetProp(MY_MODEL_POSITION, 10.0f, 10.0f, 5.0f, 0.0f);
	scene.Lights[1] = Light(MY_MODEL_LIGHT_POINT);


	GLfloat lmodel_ambient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	glEnable(GL_LIGHTING);

	
	//init scene
	scene.init();
	scene.Switch(MY_MODEL_LIGHT, 0, true);
	scene.Switch(MY_MODEL_LIGHT, 1, true);
	auto a = scene.AddSphere(1.0);
	scene.MovePos(MY_MODEL_OBJECT, a, { 0.0, -5.2, 0.0 });
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
		cam.yaw(-3);
		break;
	case GLUT_KEY_RIGHT:
		cam.yaw(3);
		break;
	case GLUT_KEY_UP:
		cam.pitch(3);
		break;
	case GLUT_KEY_DOWN:
		cam.pitch(-3);
		break;
	case GLUT_KEY_F1:
	case GLUT_KEY_F2:
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
		Mode = !Mode;
		glutPostRedisplay();
		return;
	}
	if (key == 32)//scape
	{
		Mode = false;
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
		//ray trace render
		if (!rayt.isFinish)
		{
			Mode = false;
			rayt.stop();
		}
		//start ray-trace
		else
		{
			int tnum = 8;
			switch (key)
			{
			case '1':
				Mode = false;
				rayt.start(MY_MODEL_CHECK, tnum);
				glutTimerFunc(50, onTimer, 1);
				break;
			case '2':
				Mode = false;
				rayt.start(MY_MODEL_INTERSECTION, tnum);
				glutTimerFunc(50, onTimer, 1);
				break;
			}
		}
		glutPostRedisplay();
		return;
	}
	if (!rayt.isFinish || !Mode)
		return;
	switch (key)
	{
	case 'h':
		cam.move(1, 0, 0);
		break;
	case 'f':
		cam.move(-1, 0, 0);
		break;
	case 't':
		cam.move(0, 1, 0);
		break;
	case 'g':
		cam.move(0, -1, 0);
		break;
	case 27:
		exit(0);
		return;
	case 'q'://light near
		light.move(0, 0, -1);
		break;
	case 'e'://light far
		light.move(0, 0, 1);
		break;
	case 'a':
		light.move(0, -3, 0);
		break;
	case 'd':
		light.move(0, 3, 0);
		break;
	case 'w':
		light.move(-3, 0, 0);
		break;
	case 's':
		light.move(3, 0, 0);
		break;
	case '2':
	case '4':
	case '6':
	case '8':
	case 43://+
	case 45://-
		if (obj_toggle == -1)
			return;
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
		double pdx = 10.0*dx / cam.width, pdy = 10.0*dy / cam.height;
		cam.move(-pdx, pdy, 0);
		//cam.move(pdx, pdy);
		glutPostRedisplay();
	}
}

void onMenu(int val)
{
	if (val & 0xf0)
	{//object
		int obj = (val >> 4) - 1;
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
			obj_toggle = -1;
			InitMenu();
			break;
		case 3://z-rotate
			Model &model = dynamic_cast<Model&>(*get<0>(scene.Objects[obj]));
			model.zRotate();
			break;
		}
	}
	else
	{//system
		switch (val & 0xf)
		{
		case 0://Add Sphere
			obj_toggle = scene.AddSphere(1.0);
			InitMenu();
			break;
		case 1://Add Cube
			obj_toggle = scene.AddCube(1.0);
			InitMenu();
			break;
		case 2://Add Model
			if (FileDlg(filename[0], filename[1]) == -1)
				return;
			obj_toggle = scene.AddModel(filename[0], filename[1]);
			InitMenu();
			break;
		default:
			return;
		}
	}
	glutPostRedisplay();
}

DWORD WINAPI showdata(LPVOID lpParam)
{
	HANDLE hOut;
	COORD pos = { 0,0 };
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	std::locale::global(std::locale(""));
	while (true)
	{
		SetConsoleCursorPosition(hOut, pos);
		wprintf(L"方向键移动摄像机，wasd键移动灯，+-号缩放，12键开关灯\n");

		wprintf(L"相机绝对坐标：\t%4f，%4f，%4f\n", cam.position.x, cam.position.y, cam.position.z);
		wprintf(L"相机u坐标：\t%4f，%4f，%4f\n", cam.u.x, cam.u.y, cam.u.z);
		wprintf(L"相机v坐标：\t%4f，%4f，%4f\n", cam.v.x, cam.v.y, cam.v.z);
		wprintf(L"相机n坐标：\t%4f，%4f，%4f\n", cam.n.x, cam.n.y, cam.n.z);

		wprintf(L"灯球坐标：\t%4f，%4f，%4f\n", light.angy, light.angz, light.dis);
		wprintf(L"Toggle:%3d\n", obj_toggle);
		wprintf(L"%s\n", rayt.isFinish ? L"finish" : L"runing");
		Sleep(33);
	}
	return 0;
}


int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(cam.width, cam.height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(argv[0]);
	init();
	InitMenu();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(onKeyboard);
	glutSpecialFunc(onKeyboard);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMouse);
	glutMouseWheelFunc(onWheel);

	HANDLE th = CreateThread(NULL, 0, showdata, NULL, 0, NULL);
	
	glutMainLoop();
	return 0;
}
