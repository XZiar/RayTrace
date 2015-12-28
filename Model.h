#pragma once

#include "rely.h"
#include "3DElement.h"

#define MY_MODEL_Z_ROLL 0x1

class Model : public DrawObject
{
private:
	class Loader
	{
		wstring filename;
		FILE *fp;
		char tmpdat[8][256];
	public:
		Loader(wstring filename);
		~Loader();
		int8_t read(string data[]);
		int8_t parseDouble(const string &in, GLdouble out[]);
		int8_t parseInt(const string &in, int out[]);
	};
	Vertex VerMin, VerMax, BorderMin, BorderMax;
public:
	vector<Vertex> vers;
	vector<Normal> nors;
	vector<Vertex> txcs;
	vector<vector<Triangle>> parts;
	vector<vector<Triangle>> newparts;
	vector<Vertex> borders;
	vector<Vertex> bboxs;
	vector<Material> mtls;
	vector<Texture> texs;
	vector<int8_t> obj_mtl, mtl_tex;
	wstring objname, mtlname;
private:
	virtual void GLPrepare() override;
	//double BorderTest(const Ray &ray, const Vertex &Min, const Vertex &Max);
	double TriangleTest(const Ray &ray, const Triangle &tri);
	int32_t loadobj(const wstring &objname, uint8_t code);
	int32_t loadmtl(const wstring &mtlname, uint8_t code);
	int32_t loadtex(const string &texname, uint8_t code);
	void reset();
public:
	Model(GLuint num = 0) : DrawObject(num) { };
	~Model() override;
	int32_t loadOBJ(const wstring &objname, const wstring &mtlname, uint8_t code = 0x0);
	virtual void RTPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr) override;
};


