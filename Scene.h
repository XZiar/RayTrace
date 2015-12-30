#pragma once

#include "rely.h"
#include "3DElement.h"
#include "Model.h"

#define MY_MODEL_LIGHT 0x1
#define MY_MODEL_OBJECT 0x2
#define MY_MODEL_SWITCH 0x80

class Scene
{
private:
	GLuint grdList;
public:
	Camera cam;
	Vertex EnvLight;
	vector<Light> Lights;
	vector<tuple<DrawObject*, bool>> Objects;
	Scene();
	~Scene();

	void init();
	uint8_t AddLight(const uint8_t type, const Vertex &comp, const float lumi = 1.0f, const Vertex &atte = Vertex(1, 0, 0));
	uint8_t AddSphere(float radius);
	uint8_t AddCube(float len);
	uint8_t AddModel(const wstring &objname, const wstring &mtlname, uint8_t code = 0x0);

	bool Delete(const uint8_t type, const uint8_t num);
	bool MovePos(const uint8_t type, const uint8_t num, const Vertex &v);
	bool Switch(const uint8_t type, const uint8_t num, const bool isShow);
	void DrawScene();
	void DrawLight(const uint8_t num);
};

