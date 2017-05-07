// HoT.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "Ocean.h"


int main()
{
	drw::Ocean* _ocean;
	drw::OceanContext* _ocean_context;

	int   gridres = 1 << 8;	// 256
	float stepsize = 512.f / (float)gridres;

	bool do_chop = true; 
	bool do_jacobian = true;
	bool do_normals = !true; 

	float V = 30.f; // wind speed
	float L = 1.f;	// wave size
	float A = 1.f;	// Approx. Waveheight(m)
	float W = 0.f;	// wind direction
	float damp = 1.f;	//
	float wind_align = 2.f;
	float depth = 200.f;
	float seed = 0.f;


	_ocean = new drw::Ocean(gridres, gridres, stepsize, stepsize,
			V, L, A, W, 1.f - damp, wind_align,
		depth, seed);
	float _ocean_scale = _ocean->get_height_normalize_factor();

	_ocean_context = _ocean->new_context(true, do_chop, do_normals, do_jacobian);

	float chop_amount = 1.78f;// CHOPAMOUNT(now);

	// sum up the waves at this timestep
	_ocean->update(0.f, *_ocean_context, true, do_chop, do_normals, do_jacobian,
		_ocean_scale * 1.f, chop_amount);

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

    return 0;
}

