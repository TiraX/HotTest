// HoT.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "Ocean.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include "FFTTest.h"
#include "ImageTga.h"

using namespace std;
using namespace drw;

struct vec3
{
	float x, y, z;

	vec3()
		: x(0.f), y(0.f), z(0.f)
	{}

	vec3(float _x, float _y, float _z)
		: x(_x), y(_y), z(_z)
	{}

	void normalize()
	{
		float l = 1.0f / sqrt(x * x + y * y + z * z);
		x *= l;
		y *= l;
		z *= l;
	}
};

const int grid_size = 512;
vec3 grid[grid_size][grid_size];
vec3 normal[grid_size][grid_size];

void export_as_obj()
{
	stringstream ss_v, ss_n, ss_t, ss_f;

	// vertices & normals
	for (int y = 0; y < grid_size; ++y)
	{
		for (int x = 0; x < grid_size; ++x)
		{
			const vec3& v = grid[x][y];
			ss_v << "v  " << v.x << ' ' << v.y << ' ' << v.z << "\n";

			const vec3& n = normal[x][y];
			ss_n << "n  " << n.x << ' ' << n.y << ' ' << n.z << "\n";

			const float uv = 1.f / grid_size;
			ss_t << "vt  " << uv * x << ' ' << uv * y << " 0.0000\n";
		}
	}

	// faces
	for (int y = 0; y < grid_size - 1; ++y)
	{
		for (int x = 0; x < grid_size - 1; ++x)
		{
			// 2 faces v/t/n
			int v_index = y * grid_size + x + 1;
			ss_f << "f ";
			ss_f << v_index << '/' << v_index << '/' << v_index << " ";
			ss_f << v_index + 1 << '/' << v_index + 1 << '/' << v_index + 1 << " ";
			ss_f << v_index + grid_size << '/' << v_index + grid_size << '/' << v_index + grid_size << "\n";


			ss_f << "f ";
			ss_f << v_index + grid_size + 1 << '/' << v_index + grid_size + 1 << '/' << v_index + grid_size + 1 << " ";
			ss_f << v_index + grid_size << '/' << v_index + grid_size << '/' << v_index + grid_size << " ";
			ss_f << v_index + 1 << '/' << v_index + 1 << '/' << v_index + 1 << "\n";
		}
	}

	ss_v << "# " << grid_size * grid_size << " vertices.\n\n\n";
	ss_n << "# " << grid_size * grid_size << " normals.\n\n\n";
	ss_t << "# " << grid_size * grid_size << " coords.\n\n\n";

	// faces
	ss_f << "# " << (grid_size - 1) * (grid_size - 1) * 2 << "faces.\n\n\n";


	ofstream fs("ocean.obj", ios::out | ios::trunc);
	fs << ss_v.str();
	fs << ss_t.str();
	fs << ss_n.str();
	fs << ss_f.str();
	fs.close();
}

int main()
{
	//do_fft_test();
	//return 0;

	drw::Ocean* _ocean;
	drw::OceanContext* _ocean_context;

	int   gridres = 1 << 9;	// 256
	float stepsize = grid_size / (float)gridres;

	bool do_chop = true; 
	bool do_jacobian = true;
	bool do_normals = true; 

	float V = 30.f; // wind speed
	float L = 0.1f;	// wave size
	float A = 40.f;	// Approx. Waveheight(m)
	float W = 0.f * 3.14159f / 180.f;	// wind direction
	float damp = 0.5f;	//
	float wind_align = 2.f;
	float depth = 200.f;
	float seed = 0.f;
	float chop_amount = 1.78f;// CHOPAMOUNT(now);
	float wave_scale = 20.f;
	float normal_scale = -1.f;

	// init grid & normals
	for (int y = 0 ; y < grid_size ; ++ y)
	{
		for (int x = 0; x < grid_size; ++ x)
		{
			grid[x][y] = vec3(x, 0.f, y);	// y is up vector
			normal[x][y] = vec3(0.f, 1.f, 0.f);	// y is up vector
		}
	}


	_ocean = new drw::Ocean(gridres, gridres, stepsize, stepsize,
			V, L, A, W, 1.f - damp, wind_align,
		depth, seed);
	float _ocean_scale = _ocean->get_height_normalize_factor();

	_ocean_context = _ocean->new_context(true, do_chop, do_normals, do_jacobian);


	// sum up the waves at this timestep
	_ocean->update(0.f, *_ocean_context, true, do_chop, do_normals, do_jacobian,
		_ocean_scale * 1.f, chop_amount);

	// test my fft.
	if (0)
	{
		// output image before fft
		TiImage image_tmp;
		image_tmp.w = _ocean_context->_M;
		image_tmp.h = _ocean_context->_N;
		image_tmp.data = new float[image_tmp.w * image_tmp.h];
		image_tmp.imagi = new float[image_tmp.w * image_tmp.h];
		memset(image_tmp.data, 0, image_tmp.w * image_tmp.h * 4);
		memset(image_tmp.imagi, 0, image_tmp.w * image_tmp.h * 4);

		float nmax, nmin;
		nmax = -999999999.f;
		nmin = 999999999.f;
		for (int j = 0; j <= _ocean_context->_N / 2; ++j)
		{
			// note the <= _N/2 here, see the fftw doco about
			// the mechanics of the complex->real fft storage
			int offset = j * _ocean_context->_M;
			for (int i = 0; i < _ocean_context->_M; ++i)
			{
				const complex_f& c = _ocean_context->_htilda(i, j);
				image_tmp.data[offset + i] = c.real();
				image_tmp.imagi[offset + i] = c.imag();
				if (c.real() > nmax)
					nmax = c.real();
				if (c.real() < nmin)
					nmin = c.real();

				//_ocean_context._htilda(i, j) = _h0(i, j) * exp(complex_f(0, omega(_k(i, j))*t)) +
				//	conj(_h0_minus(i, j)) * exp(complex_f(0, -omega(_k(i, j))*t));
			}
		}
		for (int j = _ocean_context->_N / 2 + 1; j < _ocean_context->_N; ++j)
		{
			// note the <= _N/2 here, see the fftw doco about
			// the mechanics of the complex->real fft storage
			int offset = j * _ocean_context->_M;
			for (int i = 0; i < _ocean_context->_M; ++i)
			{
				const complex_f& c = _ocean_context->_htilda(i, _ocean_context->_N - j - 1);
				image_tmp.data[offset + i] = c.real();
				image_tmp.imagi[offset + i] = -c.imag();

				//_ocean_context._htilda(i, j) = _h0(i, j) * exp(complex_f(0, omega(_k(i, j))*t)) +
				//	conj(_h0_minus(i, j)) * exp(complex_f(0, -omega(_k(i, j))*t));
			}
		}

		for (int i = 0; i < image_tmp.w * image_tmp.h; i++)
		{
			//image_tmp.data[i] *= _ocean_scale;
		}




		// do my transform
		TiImage my_trans;
		ifft_image(image_tmp, my_trans);

		// save locally
		float f = 1.f / (nmax - nmin);
		for (int i = 0; i < image_tmp.w * image_tmp.h; i++)
		{
			image_tmp.data[i] = (image_tmp.data[i] - nmin) * f;
		}
		saveToImage("test_before_fft.tga", image_tmp);
	}

	/*
	printf("mine.\n");
	for (int j = 0; j < my_trans.h; j++)
	{
		for (int i = 0; i < my_trans.w; i += 8)
		{
			printf("%f %f %f %f %f %f %f %f \n",
				my_trans.data[j * my_trans.w + i + 0],
				my_trans.data[j * my_trans.w + i + 1],
				my_trans.data[j * my_trans.w + i + 2],
				my_trans.data[j * my_trans.w + i + 3],
				my_trans.data[j * my_trans.w + i + 4],
				my_trans.data[j * my_trans.w + i + 5],
				my_trans.data[j * my_trans.w + i + 6],
				my_trans.data[j * my_trans.w + i + 7]);
		}
		printf("==============\n");
	}


	printf("theirs.\n");
	for (int j = 0; j < grid_size; j++)
	{
		for (int i = 0; i < grid_size; i += 8)
		{
			printf("%f %f %f %f %f %f %f %f \n", 
				_ocean_context->_disp_y(i + 0, j),
				_ocean_context->_disp_y(i + 1, j),
				_ocean_context->_disp_y(i + 2, j),
				_ocean_context->_disp_y(i + 3, j),
				_ocean_context->_disp_y(i + 4, j),
				_ocean_context->_disp_y(i + 5, j),
				_ocean_context->_disp_y(i + 6, j),
				_ocean_context->_disp_y(i + 7, j));
		}
		printf("==============\n");
	}
	//*/

	// for each vertex, calculate vertices
	for (int y = 0; y < grid_size; ++y)
	{
		for (int x = 0; x < grid_size; ++x)
		{
			vec3& v = grid[x][y];
			_ocean_context->eval2_xz(v.x, v.z);
			v = vec3(v.x + _ocean_context->disp[0] * wave_scale, v.y + _ocean_context->disp[1] * wave_scale, v.z + _ocean_context->disp[2]);
			//v = vec3(v.x, v.y + _ocean_context->disp[1] * wave_scale, v.z);
			//v = vec3(v.x, v.y + my_trans.data[y * my_trans.w + x] * wave_scale * 2.f, v.z);

			vec3& n = normal[x][y];
			n = vec3(_ocean_context->normal[0] * normal_scale, _ocean_context->normal[1] * normal_scale, _ocean_context->normal[2] * normal_scale);
			n.normalize();
		}
	}

	//bool linterp = !INTERP(now);


	//// get our attribute indices
	//GA_RWAttributeRef normal_index;
	//GA_RWAttributeRef jminus_index;
	//GA_RWAttributeRef eminus_index;

	//if (do_normals)
	//{
	//	normal_index = gdp->addNormalAttribute(GEO_POINT_DICT);
	//}
	//if (do_jacobian)
	//{
	//	// jminus_index = gdp->addPointAttrib("mineigval",sizeof(float),GB_ATTRIB_FLOAT,0);
	//	// eminus_index = gdp->addPointAttrib("mineigvec",sizeof(UT_Vector3),GB_ATTRIB_VECTOR,0);
	//	jminus_index = gdp->addTuple(GA_STORE_REAL32, GA_ATTRIB_POINT, "mineigval", 1, GA_Defaults(0));
	//	eminus_index = gdp->addFloatTuple(GA_ATTRIB_POINT, "mineigvec", 3, GA_Defaults(0));
	//}

	//// this is not that fast, can it be done quicker ???
	//GA_FOR_ALL_GPOINTS(gdp, ppt)
	//{
	//	UT_Vector4 p = ppt->getPos();


	//	if (linterp)
	//	{
	//		_ocean_context->eval_xz(p(0), p(2));
	//	}
	//	else
	//	{
	//		_ocean_context->eval2_xz(p(0), p(2));
	//	}

	//	if (do_chop)
	//	{
	//		p.assign(p(0) + _ocean_context->disp[0],
	//			p(1) + _ocean_context->disp[1],
	//			p(2) + _ocean_context->disp[2]);
	//	}
	//	else
	//	{
	//		p.assign(p(0), p(1) + _ocean_context->disp[1], p(2));
	//	}

	//	if (do_normals)
	//	{
	//		/*

	//		UT_Vector3* normal = (UT_Vector3*) ppt->castAttribData<UT_Vector3>(normal_index);
	//		normal->assign(_ocean_context->normal[0],
	//		_ocean_context->normal[1],
	//		_ocean_context->normal[2]);
	//		normal->normalize();
	//		*/
	//		ppt->getValue<UT_Vector3>(normal_index).assign(_ocean_context->normal[0],
	//			_ocean_context->normal[1],
	//			_ocean_context->normal[2]);
	//		ppt->getValue<UT_Vector3>(normal_index).normalize();
	//	}

	//	if (do_jacobian)
	//	{/*
	//	 float *js = (float*)ppt->castAttribData<float>(jminus_index);
	//	 *js = _ocean_context->Jminus;
	//	 UT_Vector3* eminus = (UT_Vector3*)ppt->castAttribData<UT_Vector3>(eminus_index);
	//	 eminus->assign(_ocean_context->Eminus[0],0,_ocean_context->Eminus[1]);
	//	 */
	//		ppt->setValue<float>(jminus_index, _ocean_context->Jminus);
	//		ppt->setValue<UT_Vector3>(eminus_index, UT_Vector3(_ocean_context->Eminus[0],
	//			_ocean_context->Eminus[1],
	//			_ocean_context->Eminus[2]));

	//	}
	//	ppt->setPos(p);
	//}

	export_as_obj();

    return 0;
}

