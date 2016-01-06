#pragma once

#include "rely.h"
#include "3DElement.h"
#include "Basic3DObject.h"
#include "Model.h"

#define MY_MODEL_LIGHT  0x1
#define MY_MODEL_OBJECT 0x2
#define MY_LIGHT_COMPENT 0x1
#define MY_LIGHT_LUMI 0x2
#define MY_OBJECT_MATERIAL 0x1
#define MY_OBJECT_COLOR 0x2
#define MY_MODEL_SWITCH 0x80

class Scene
{
private:
	GLuint grdList;
public:
	vector<Material> MtlLiby;

	Camera cam;
	Vertex EnvLight;
	vector<Light> Lights;
	vector<DrawObject*> Objects;
	Scene();
	~Scene();

	uint8_t AddLight(const uint8_t type, const Vertex &comp, const Vertex &atte = Vertex(1, 0, 0, 1));
	uint8_t AddSphere(const float radius);
	uint8_t AddCube(const float len);
	uint8_t AddModel(const wstring &objname, const wstring &mtlname, uint8_t code = 0x0);
	uint8_t AddPlane();
	uint8_t AddBallPlane(const float radius);

	bool Delete(const uint8_t type, const uint8_t num);
	bool ChgLightComp(const uint8_t type, const uint8_t num, const Vertex &v);
	bool ChgMtl(const uint8_t num, const Material &mtl);
	bool ChgMtl(const uint8_t num, const Normal &clr);
	bool MovePos(const uint8_t type, const uint8_t num, const Vertex &v);
	bool Switch(const uint8_t type, const uint8_t num, const bool isShow);
	void DrawScene();
	void DrawLight();
};

