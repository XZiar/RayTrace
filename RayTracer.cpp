#include "rely.h"
#include "RayTracer.h"


void RayTracer::RTcheck(const int8_t tNum, const int8_t tID)
{
	int32_t blk_h = height / 64, blk_w = width / 64;
	for (int16_t blk_xcur = tID,blk_ycur = 0; blk_ycur < blk_h;)
	{
		uint8_t *out_cur = output + blk_ycur * 64 * (3 * width) + blk_xcur * 64 * 3;
		Color c((blk_ycur & 0x1) == (blk_xcur & 0x1));
		for (auto line_ycur = 0; line_ycur < 64; ++line_ycur)
		{
			for (auto line_xcur = 0; line_xcur < 64; ++line_xcur)
			{
				if (!isRun)
				{
					state[tID] = true;
					return;
				}
				c.put(out_cur);
				out_cur += 3;
			}
			out_cur += (width - 64) * 3;
			Sleep(tNum);
		}
		blk_xcur += tNum;
		if (blk_xcur >= blk_w)
		{
			blk_ycur += blk_xcur / blk_w;
			blk_xcur = blk_xcur % blk_w;
		}
	}
	state[tID] = true;
}

void RayTracer::RTintersection(int8_t tNum, int8_t tID)
{
	int32_t blk_h = height / 64, blk_w = width / 64;
	Camera &cam = scene->cam;
	Ray baseray(cam.position, cam.n);

	double dp = tan(cam.fovy * PI / 360) / (height / 2);
	Color c_bg(true);
	c_bg.g = 255;

	for (int16_t blk_xcur = tID, blk_ycur = 0; blk_ycur < blk_h;)//per unit
	{
		uint8_t *out_cur = output + blk_ycur * 64 * (3 * width) + blk_xcur * 64 * 3;
		
		for (auto ycur = blk_ycur * 64 - height / 2, ymax = ycur + 64; ycur < ymax; ++ycur)//pur y-line
		{
			for (auto xcur = blk_xcur * 64 - width / 2, xmax = xcur + 64; xcur < xmax; ++xcur)//per pixel
			{
				if (!isRun)
				{
					state[tID] = true;
					return;
				}
				
				Vertex dir = cam.n + cam.u*(xcur*dp) + cam.v*(ycur*dp);
				Ray ray(cam.position, dir);

				HitRes hr;
				for (auto t : scene->Objects)
				{
					if (get<1>(t))
						hr = get<0>(t)->intersect(ray, hr);
				}
				if (hr)
				{
					Color c(hr.distance, 1, 20);
					c.put(out_cur);
				}
				else
					c_bg.put(out_cur);
				out_cur += 3;
			}
			out_cur += (width - 64) * 3;
			//Sleep(1);
		}
		blk_xcur += tNum;
		if (blk_xcur >= blk_w)
		{
			blk_ycur += blk_xcur / blk_w;
			blk_xcur = blk_xcur % blk_w;
		}
	}
	state[tID] = true;
}

void RayTracer::RTthread(const int8_t tNum, const int8_t tID)
{
	Sleep(2000);
	state[tID] = true;
}



RayTracer::RayTracer(Scene &scene)
{
	this->scene = &scene;
	output = new uint8_t[2048 * 2048 * 3];
	memset(output, 127, 2048 * 2048 * 3);
	glGenTextures(1, &texID);
}


RayTracer::~RayTracer()
{
	delete[] output;
}

/*const function<void(void)>&refresh, */
void RayTracer::start(const uint8_t type, const int8_t tnum)
{
	isFinish = false;
	isRun = true;
	width = scene->cam.width;
	height = scene->cam.height;
	memset(output, 127, 2048 * 2048 * 3);
	for (auto t : scene->Objects)
	{
		if (get<1>(t))
			get<0>(t)->RTPrepare();
	}
	for (int8_t a = 0; a < tnum; a++)
	{
		state[a] = false;
		switch (type)
		{
		case MY_MODEL_CHECK:
			t[a] = thread(mem_fn(&RayTracer::RTcheck), this, tnum, a);
			break;
		case MY_MODEL_INTERSECTION:
			t[a] = thread(mem_fn(&RayTracer::RTintersection), this, tnum, a);
			break;
		case MY_MODEL_RAYTRACE:
			t[a] = thread(mem_fn(&RayTracer::RTthread), this, tnum, a);
			break;
		}
		
		t[a].detach();
	}
	auto tmain = thread([&, tnum]
	{
		while (true)
		{
			bool isOK = true;
			for (int8_t a = 0; a < tnum; a++)
				if (state[a] == false)
				{
					isOK = false;
					break;
				}
			if (isOK)
				break;
			Sleep(50);
		}
		isFinish = true;
		isRun = false;
	});
	tmain.detach();
}

void RayTracer::stop()
{
	isRun = false;
}
