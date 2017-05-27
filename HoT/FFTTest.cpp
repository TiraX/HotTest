// HoT.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FFTTest.h"

#include <vector>
#include <windows.h>

using namespace std;

const float PI = 3.14159265f;
const float PI2 = PI * 2.f;

class TiComplex
{
public:
	TiComplex()
		: A(0.f)
		, B(0.f)
	{}
	TiComplex(float a)
		: A(a)
		, B(0.f)
	{}
	TiComplex(float a , float b)
		: A(a) 
		, B(b)
	{}
	~TiComplex()
	{}

	TiComplex& operator = (const TiComplex& o)
	{
		A = o.A;
		B = o.B;
		return *this;
	}

	TiComplex& operator += (const TiComplex& o)
	{
		A += o.A;
		B += o.B;
		return *this;
	}

public:
	float A, B;
};

typedef std::vector<TiComplex> vec;

TiComplex operator + (const TiComplex& a, const TiComplex& b)
{
	TiComplex r;
	r.A = a.A + b.A;
	r.B = a.B + b.B;
	return r;
}

TiComplex operator - (const TiComplex& a, const TiComplex& b)
{
	TiComplex r;
	r.A = a.A - b.A;
	r.B = a.B - b.B;
	return r;
}

TiComplex operator * (const TiComplex& a, const TiComplex& b)
{
	TiComplex r;
	r.A = a.A * b.A - a.B * b.B;
	r.B = a.A * b.B + b.A * a.B;
	return r;
}

vec dft(const vec& a)
{
	int n = a.size();
	TiComplex w;
	vec result;
	for (int j = 0 ; j < n; ++ j)
	{
		w = TiComplex(cos(PI2 * j / n), sin(PI2 * j / n));
		TiComplex r = w * a[n - 1];
		for (int i = n - 1; i > 1; --i)
		{
			r = w*(a[i - 1] + r);
		}
		r = r + a[0];
		result.push_back(r);
	}
	return result;
}


vec recursive_fft(const vec& a)
{
	vec y;
	int n = a.size();
	if (n == 1)
		return a;
	TiComplex wn, w;
	wn = TiComplex(cos(PI2 / n), sin(PI2 / n));
	w = TiComplex(1, 0);

	vec a0, a1;
	for (int i = 0 ; i < n ; i += 2)
	{
		a0.push_back(a[i]);
	}
	for (int i = 1; i < n; i += 2)
	{
		a1.push_back(a[i]);
	}

	vec y0 = recursive_fft(a0);
	vec y1 = recursive_fft(a1);

	y.resize(n);
	int m = n / 2;
	for (int k = 0 ; k < m; ++ k)
	{
		TiComplex t = w * y1[k];
		y[k] = y0[k] + t;
		y[k + m] = y0[k] - t;
		w = w * wn;
	}

	return y;
}


void print_vec(const vec& v)
{
	int n = v.size();
	printf("n = %d\n---------------\n", n);
	for (int i = 0 ; i < n; ++ i)
	{
		printf("  %f + i * %f\n", v[i].A, v[i].B);
	}
}

void time_test(const vec& a)
{
	const int times = 1000;
	long long t_start, t_end;
	vec y;
	t_start = timeGetTime();
	for (int i = 0; i < times; ++i)
	{
		y = dft(a);
	}
	t_end = timeGetTime();
	printf("dft time is %lld.\n", t_end - t_start);
	t_start = timeGetTime();
	for (int i = 0; i < times; ++i)
	{
		y = recursive_fft(a);
	}
	t_end = timeGetTime();
	printf("fft time is %lld.\n", t_end - t_start);
}

void do_fft_test()
{
	int size = 512;
	vec a;
	a.clear();
	for (int i = 0 ; i < size; ++ i)
	{
		TiComplex c(i + 1);
		a.push_back(c);
	}

	vec y_dft = dft(a);
	vec y_fft = recursive_fft(a);

	//print_vec(y_dft);
	//print_vec(y_fft);

	time_test(a);
}