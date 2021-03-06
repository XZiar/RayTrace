#pragma once

#include "rely.h"
#include "3DElement.h"

class Model : public DrawObject
{
private:
	class Loader
	{
		wstring filename;
		FILE *fp;
		char tmpdat[8][256];
	public:
		Loader(const wstring &fname);
		~Loader();
		int8_t read(string data[]);
		int8_t parseFloat(const string &in, float out[]);
		int8_t parseInt(const string &in, int out[]);
	};
	GLuint texList[64];
	Vertex VerMin, VerMax, BorderMin, BorderMax;
public:
	vector<Vertex> vers;
	vector<Normal> nors;
	vector<Coord2D> txcs;
	vector<vector<Triangle>> parts;
	//vector<vector<clTri>> clparts;
	vector<vector<clTri>> octclparts;
	vector<Vertex> borders;
	vector<Vertex> bboxs;
	vector<Material> mtls;
	vector<Texture> texs;
	vector<int8_t> part_mtl, mtl_tex;
	wstring objname, mtlname;
private:
	virtual void GLPrepare() override;
	int32_t loadobj(const wstring &objname, const uint8_t code);
	int32_t loadmtl(const wstring &mtlname, const uint8_t code);
	int32_t loadtex(const string &texname, const uint8_t code);
	void reset();
public:
	Model(GLuint num = 0) : DrawObject(num) { type = MY_OBJECT_MODEL; };
	~Model() override;
	int32_t loadOBJ(const wstring &objname, const wstring &mtlname, const uint8_t code = 0x0);
	void zRotate();
	virtual void SetMtl(const Material &mtl) override;
	virtual void RTPrepare() override;
	virtual HitRes intersect(const Ray &ray, const HitRes &hr, const float min = 0) override;
};


