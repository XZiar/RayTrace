#include "rely.h"
#include "Scene.h"



Scene::Scene()
{
	EnvLight = Vertex(0.2f, 0.2f, 0.2f, 1.0f);
	for (Light &light : Lights)
		light.bLight = false;
	//init material library
	Material mtl;
	mtl.name = "brass";//黄铜材质
	mtl.SetMtl(MY_MODEL_AMBIENT, 0.329412f, 0.223529f, 0.027451f);
	mtl.SetMtl(MY_MODEL_DIFFUSE, 0.780392f, 0.568627f, 0.113725f);
	mtl.SetMtl(MY_MODEL_SPECULAR, 0.992157f, 0.941176f, 0.807843f);
	mtl.SetMtl(MY_MODEL_SHINESS, 27.8974f);
	MtlLiby.push_back(mtl);

	mtl = Material();
	mtl.name = "bas-sphere";//基本球材质
	mtl.SetMtl(MY_MODEL_AMBIENT, 0.1f, 0.1f, 0.1f);
	mtl.SetMtl(MY_MODEL_DIFFUSE, 0.1f, 0.5f, 0.8f);
	mtl.SetMtl(MY_MODEL_SPECULAR, 1.0f, 1.0f, 1.0f);
	mtl.SetMtl(MY_MODEL_SHINESS, 100);
	mtl.reflect = 0.35f;
	MtlLiby.push_back(mtl);

	mtl = Material();
	mtl.name = "mirror";//全反射
	mtl.SetMtl(MY_MODEL_AMBIENT, 0.1f, 0.1f, 0.1f);
	mtl.SetMtl(MY_MODEL_DIFFUSE, 0.1f, 0.1f, 0.1f);
	mtl.SetMtl(MY_MODEL_SPECULAR, 1.0f, 1.0f, 1.0f);
	mtl.SetMtl(MY_MODEL_SHINESS, 127);
	mtl.reflect = 0.95f;
	MtlLiby.push_back(mtl);

	mtl = Material();
	mtl.name = "green grass";//带颜色反射
	mtl.SetMtl(MY_MODEL_AMBIENT, 0.1f, 0.4f, 0.1f);
	mtl.SetMtl(MY_MODEL_DIFFUSE, 0.1f, 0.5f, 0.1f);
	mtl.SetMtl(MY_MODEL_SPECULAR, 1.0f, 1.0f, 1.0f);
	mtl.SetMtl(MY_MODEL_SHINESS, 127);
	mtl.reflect = 0.55f;
	MtlLiby.push_back(mtl);

	mtl = Material();
	mtl.name = "grass";//全反射
	mtl.SetMtl(MY_MODEL_AMBIENT, 0.1f, 0.1f, 0.1f);
	mtl.SetMtl(MY_MODEL_DIFFUSE, 0.1f, 0.1f, 0.1f);
	mtl.SetMtl(MY_MODEL_SPECULAR, 1.0f, 1.0f, 1.0f);
	mtl.SetMtl(MY_MODEL_SHINESS, 127);
	mtl.reflect = 0.15f;
	mtl.refract = 0.75f; mtl.rfr = 1.5f;
	MtlLiby.push_back(mtl);
}

Scene::~Scene()
{
	for (auto &obj : Objects)
		if(obj != nullptr)
			delete obj;
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

uint8_t Scene::AddSphere(const float radius)
{
	GLuint lnum = glGenLists(1);
	Sphere *sphere = new Sphere(radius, lnum);
	sphere->position = Vertex(0.0, radius, 0.0);
	sphere->SetMtl(MtlLiby[1]);
	sphere->GLPrepare();

	Objects.push_back(sphere);
	return Objects.size() - 1;
}

uint8_t Scene::AddCube(const float len)
{
	GLuint lnum = glGenLists(1);
	Box *box = new Box(len, lnum);
	box->position = Vertex(0.0, len / 2, 0.0);
	box->SetMtl(MtlLiby[0]);//黄铜
	box->GLPrepare();

	Objects.push_back(box);
	return Objects.size() - 1;
}

uint8_t Scene::AddModel(const wstring & objname, const wstring & mtlname, uint8_t code)
{
	GLuint lnum = glGenLists(1);
	Model *model = new Model(lnum);
	model->loadOBJ(objname, mtlname, code);

	Objects.push_back(model);
	return Objects.size() - 1;
}

uint8_t Scene::AddPlane()
{
	GLuint lnum = glGenLists(1);
	Plane *plane = new Plane(lnum);
	Material mtl;
	mtl.reflect = 0.8;
	plane->SetMtl(mtl);
	plane->GLPrepare();

	Objects.push_back(plane);
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
	case MY_LIGHT_COMPENT:
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
	case MY_LIGHT_LUMI:
		tmp = light.attenuation.alpha * v.alpha;
		light.SetLumi(tmp);
		break;
	default:
		return false;
	}
	return true;
}

bool Scene::ChgMtl(const uint8_t num, const Material &mtl)
{
	if (num >= Objects.size())
		return false;
	Objects[num]->SetMtl(mtl);
	Objects[num]->GLPrepare();
	return true;
}

bool Scene::ChgMtl(const uint8_t num, const Normal &clr)
{
	if (num >= Objects.size())
		return false;
	auto chgclr = [clr](Vertex &mclr) 
	{
		if (clr.w < 0.5f)
		{
			float sum = mclr.r + mclr.g + mclr.b;
			mclr = clr * sum;
		}
		else//rewrite color
			mclr = clr;
	};
	if (Objects[num]->type == MY_OBJECT_MODEL)
	{
		Model &m = dynamic_cast<Model&>(*Objects[num]);
		for (Material &mtl : m.mtls)
			chgclr(mtl.diffuse);
	}
	else
	{
		chgclr(Objects[num]->mtl.diffuse);
	}
	Objects[num]->GLPrepare();
	return true;
}

bool Scene::Delete(uint8_t type, const uint8_t num)
{
	switch (type)
	{
	case MY_MODEL_OBJECT:
		if (num >= Objects.size())
			return false;
		delete Objects[num];
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
		if (Objects[num]->type == MY_OBJECT_PLANE)
		{
			Plane &p = dynamic_cast<Plane&>(*Objects[num]);
			//p.position += p.normal * -v.z;
			p.rotate(v);
			p.GLPrepare();
		}
		else
			Objects[num]->position += v;
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
		old = Objects[num]->bShow;
		Objects[num]->bShow = isSwitch ? !old : isShow;
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
	for (auto obj : Objects)
	{
		if (obj->bShow)
		{
			Vertex &pos = obj->position;
			glPushMatrix();
			glTranslated(pos.x, pos.y, pos.z);
			obj->GLDraw();
			glPopMatrix();
		}
	}

	//draw ground
	glPushMatrix();
	glCallList(grdList);
	glPopMatrix();

	glPopMatrix();
}

void Scene::DrawLight()
{
	Vertex lgt(1, 1, 1);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, lgt);
	for(Light &light:Lights)
		if (light.bLight)
		{
			glPushMatrix();
			glTranslatef(light.position.x, light.position.y, light.position.z);
			glutWireSphere(0.1, 10, 10);
			glPopMatrix();
		}
}
