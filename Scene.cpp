#include "Scene.h"



Scene::Scene()
{
	for (Light &light : Lights)
		light.bLight = false;
}


Scene::~Scene()
{
	for (auto &obj : Objects)
	{
		auto a = get<0>(obj);
		if(a != nullptr)
			delete a;
	}
}

void Scene::init()
{
	//init draw ground
	grdList = glGenLists(1);
	glNewList(grdList, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, 0);
	GLfloat no_mat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	GLfloat emission[] = { 0.0f, 0.0f, 0.5f, 0.0f };
	glBegin(GL_LINES);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, no_mat);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, no_mat);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, no_mat);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
	for (int a = -1000; a < 1001; a += 5)
	{
		glVertex3i(a, 0, 1000);
		glVertex3i(a, 0, -1000);
		glVertex3i(-1000, 0, a);
		glVertex3i(1000, 0, a);
	}
	glEnd();
	glEndList();
}

int8_t Scene::AddSphere(double radius)
{
	Material mtl;
	mtl.name = "Sphere";
	mtl.SetMtl(MY_MODEL_AMBIENT, 0.1, 0.1, 0.1);
	mtl.SetMtl(MY_MODEL_DIFFUSE, 0.1, 0.5, 0.8);
	mtl.SetMtl(MY_MODEL_SPECULAR, 1.0, 1.0, 1.0);
	mtl.SetMtl(MY_MODEL_SHINESS, 100, 100, 100);

	GLuint lnum = glGenLists(1);
	Sphere *sphere = new Sphere(radius, lnum);
	sphere->SetMtl(mtl);
	sphere->GLPrepare();

	Objects.push_back(make_tuple(sphere, true));
	return Objects.size() - 1;
}

int8_t Scene::AddModel(const wstring & objname, const wstring & mtlname, uint8_t code)
{
	GLuint lnum = glGenLists(1);
	Model *model = new Model(lnum);
	model->loadOBJ(objname, mtlname, code);

	Objects.push_back(make_tuple(model, true));
	return Objects.size() - 1;
}

bool Scene::SetPos(const int8_t &num, const Vertex & v)
{
	if (num >= Objects.size())
		return false;
	get<0>(Objects[num])->position = v;

	return true;
}

bool Scene::Switch(uint8_t type, const int8_t &num, const bool &isShow)
{
	bool old, isSwitch = type & MY_MODEL_SWITCH;
	switch (type & 0x7f)
	{
	case MY_MODEL_LIGHT:
		if (num > 7)
			return false;
		old = Lights[num].bLight;
		Lights[num].bLight = isSwitch ? !Lights[num].bLight : isShow;
		return true;
	case MY_MODEL_OBJECT:
		if (num >= Objects.size())
			return false;
		old = get<1>(Objects[num]);
		get<1>(Objects[num]) = isSwitch ? !old : isShow;
		return old;
	}
	return false;
}

void Scene::DrawScene()
{
	//set camera
	/*gluLookAt(cam.position.x, cam.position.y, cam.position.z,
		cam.poi.x, cam.poi.y, cam.poi.z,
		cam.head.x, cam.head.y, cam.head.z);*/
	Vertex poi = cam.position + cam.n;
	gluLookAt(cam.position.x, cam.position.y, cam.position.z,
		poi.x, poi.y, poi.z,
		cam.v.x, cam.v.y, cam.v.z);

	glPushMatrix();
	glTranslatef(0, -4, 0);

	//put light
	for (auto a = 0; a < 8; ++a)
	{
		GLenum ID = GL_LIGHT0 + a;
		Light &light = Lights[a];
		if(light.bLight)
		{
			glLightfv(ID, GL_AMBIENT, light.ambient);
			glLightfv(ID, GL_SPECULAR, light.specular);
			glLightfv(ID, GL_DIFFUSE, light.diffuse);
			glLightfv(ID, GL_POSITION, light.position);
			if (light.position[3] > 0.5)
			{
				glLightf(ID, GL_QUADRATIC_ATTENUATION, light.attenuation[2]);
				glLightf(ID, GL_LINEAR_ATTENUATION, light.attenuation[1]);
				glLightf(ID, GL_CONSTANT_ATTENUATION, light.attenuation[0]);
			}
			glEnable(ID);
		}
		else
			glDisable(ID);
	}
	
	//draw object
	DrawObject *dobj;
	bool isDraw;
	for (auto a = 0; a < Objects.size(); ++a)
	{
		tie(dobj, isDraw) = Objects[a];
		if (isDraw)
		{
			Vertex &pos = dobj->position;
			glPushMatrix();
			glTranslated(pos.x, pos.y, pos.z);
			dobj->GLDraw();
			glPopMatrix();
		}
	}

	//draw ground
	glPushMatrix();
	glCallList(grdList);
	glPopMatrix();

	glPopMatrix();
}
