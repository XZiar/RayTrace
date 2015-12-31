#include "rely.h"
#include "Scene.h"



Scene::Scene()
{
	EnvLight = Vertex(0.2f, 0.2f, 0.2f, 1.0f);
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
	glPushMatrix();
	glTranslatef(0, -5, 0);
	glBegin(GL_LINES);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, no_mat);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, no_mat);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, no_mat);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, no_mat);
	for (int a = -1000; a < 1001; a += 5)
	{
		glVertex3i(a, 0, 1000);
		glVertex3i(a, 0, -1000);
		glVertex3i(-1000, 0, a);
		glVertex3i(1000, 0, a);
	}
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, no_mat);
	glEnd();
	glPopMatrix();
	glEndList();
}

uint8_t Scene::AddLight(const uint8_t type, const Vertex &comp, const Vertex &atte)
{
	if (Lights.size() == 8)
		return 0xff;
	Light light(type);
	float sum = comp.x + comp.y + comp.z;
	Vertex nc = comp / sum;
	light.SetProperty(MY_MODEL_AMBIENT, nc.x, nc.x, nc.x);
	light.SetProperty(MY_MODEL_DIFFUSE, nc.y, nc.y, nc.y);
	light.SetProperty(MY_MODEL_SPECULAR, nc.z, nc.z, nc.z);
	light.SetProperty(MY_MODEL_ATTENUATION, atte.x, atte.y, atte.z);
	light.SetLumi(atte.alpha);
	Lights.push_back(light);
	return Lights.size() - 1;
}

uint8_t Scene::AddSphere(float radius)
{
	Material mtl;
	mtl.name = "Sphere";
	mtl.SetMtl(MY_MODEL_AMBIENT, 0.1, 0.1, 0.1);
	mtl.SetMtl(MY_MODEL_DIFFUSE, 0.1, 0.5, 0.8);
	mtl.SetMtl(MY_MODEL_SPECULAR, 1.0, 1.0, 1.0);
	int a = Objects.size() * 10;
	mtl.SetMtl(MY_MODEL_SHINESS, 100-a, 100-a, 100-a);

	GLuint lnum = glGenLists(1);
	Sphere *sphere = new Sphere(radius, lnum);
	sphere->position = Vertex(0.0, radius, 0.0);
	sphere->SetMtl(mtl);
	sphere->GLPrepare();

	Objects.push_back(make_tuple(sphere, true));
	return Objects.size() - 1;
}

uint8_t Scene::AddCube(float len)
{
	Material mtl;
	mtl.name = "Cube";
	//»ÆÍ­²ÄÖÊ
	mtl.SetMtl(MY_MODEL_AMBIENT, 0.329412, 0.223529, 0.027451);
	mtl.SetMtl(MY_MODEL_DIFFUSE, 0.780392, 0.568627, 0.113725);
	mtl.SetMtl(MY_MODEL_SPECULAR, 0.992157, 0.941176, 0.807843);
	mtl.SetMtl(MY_MODEL_SHINESS, 27.897400, 27.897400, 27.897400);

	GLuint lnum = glGenLists(1);
	Box *box = new Box(len, lnum);
	box->position = Vertex(0.0, len / 2, 0.0);
	box->SetMtl(mtl);
	box->GLPrepare();

	Objects.push_back(make_tuple(box, true));
	return Objects.size() - 1;
}

uint8_t Scene::AddModel(const wstring & objname, const wstring & mtlname, uint8_t code)
{
	GLuint lnum = glGenLists(1);
	Model *model = new Model(lnum);
	model->loadOBJ(objname, mtlname, code);

	Objects.push_back(make_tuple(model, true));
	return Objects.size() - 1;
}

bool Scene::ChgLightComp(const uint8_t type, const uint8_t num, const Vertex & v)
{
	if (num >= Lights.size())
		return false;
	Light &light = Lights[num];
	Vertex ta, td, ts;
	float tmp;
	switch (type)
	{
	case MY_MODEL_SWITCH:
		ta = light.ambient * v.x;
		td = light.diffuse * v.y;
		ts = light.specular * v.z;
		tmp = (ta.x + td.x + ts.x) / light.attenuation.alpha;
		ta.x /= tmp, ts.x /= tmp, td.x /= tmp;
		tmp = (ta.y + td.y + ts.y) / light.attenuation.alpha;
		ta.y /= tmp, ts.y /= tmp, td.y /= tmp;
		tmp = (ta.z + td.z + ts.z) / light.attenuation.alpha;
		ta.z /= tmp, ts.z /= tmp, td.z /= tmp;
		light.SetProperty(MY_MODEL_AMBIENT, ta.x, ta.y, ta.z);
		light.SetProperty(MY_MODEL_DIFFUSE, td.x, td.y, td.z);
		light.SetProperty(MY_MODEL_SPECULAR, ts.x, ts.y, ts.z);
		break;
	case MY_MODEL_ATTENUATION:
		tmp = light.attenuation.alpha * v.alpha;
		light.SetLumi(tmp);
		break;
	default:
		return false;
	}
	return true;
}

bool Scene::Delete(uint8_t type, const uint8_t num)
{
	switch (type)
	{
	case MY_MODEL_OBJECT:
		if (num >= Objects.size())
			return false;
		delete get<0>(Objects[num]);
		Objects.erase(Objects.begin() + num);
		break;
	case MY_MODEL_LIGHT:
		if (num >= Lights.size())
			return false;
		Lights.erase(Lights.begin() + num);
	}
	return true;
}

bool Scene::MovePos(const uint8_t type, const uint8_t num, const Vertex & v)
{
	switch (type)
	{
	case MY_MODEL_OBJECT:
		if (num >= Objects.size())
			return false;
		get<0>(Objects[num])->position += v;
		break;
	case MY_MODEL_LIGHT:
		if (num >= Lights.size())
			return false;
		Lights[num].move(v.x, v.y, v.z);
		break;
	default:
		return false;
	}
	return true;
}

bool Scene::Switch(uint8_t type, const uint8_t num, const bool isShow)
{
	bool old, isSwitch = type & MY_MODEL_SWITCH;
	switch (type & 0x7f)
	{
	case MY_MODEL_LIGHT:
		if (num >= Lights.size())
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
	Vertex poi = cam.position + cam.n;
	gluLookAt(cam.position.x, cam.position.y, cam.position.z,
		poi.x, poi.y, poi.z,
		cam.v.x, cam.v.y, cam.v.z);

	glPushMatrix();

	//put environment light
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, EnvLight);
	
	//put light
	for (auto a = 0; a < Lights.size(); ++a)
	{
		GLenum ID = GL_LIGHT0 + a;
		Light &light = Lights[a];
		if(light.bLight)
		{
			glLightfv(ID, GL_AMBIENT, light.ambient);
			glLightfv(ID, GL_SPECULAR, light.specular);
			glLightfv(ID, GL_DIFFUSE, light.diffuse);
			glLightfv(ID, GL_POSITION, light.position);
			glLightf(ID, GL_CONSTANT_ATTENUATION, light.attenuation.x);
			glLightf(ID, GL_LINEAR_ATTENUATION, light.attenuation.y);
			glLightf(ID, GL_QUADRATIC_ATTENUATION, light.attenuation.z);
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

void Scene::DrawLight(const uint8_t num)
{
	if (num >= Lights.size())
		return;
	Vertex lgt(1, 1, 1);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, lgt);
	Light &light = Lights[num];
	if (light.bLight)
	{
		glPushMatrix();
		glTranslatef(light.position.x, light.position.y, light.position.z);
		glutWireSphere(0.1, 10, 10);
		glPopMatrix();
	}
}
