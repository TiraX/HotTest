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

vector<int> bf;

int rev_bit(int k, int n)
{
	int k0 = 0, m = log((double)n) / log((double)2);
	for (int j = 0; j < m; ++j)
	{
		if (k&(1 << j))k0 += (1 << (m - 1 - j));
	}
	return k0;
}

void bitReverseCopy(const vec& src, vec &dest)
{
	int n = src.size();
	for (int k = 0; k <= n - 1; ++k) dest[bf[k]] = src[k];
}
vec iteration_fft(const vec& a1)
{
	vec y;

	vec a;
	a.resize(a1.size());
	bitReverseCopy(a1, a);

	int n = a.size();
	int s, m, k, j, mh;
	int sn = log2(n);
	m = 1;
	y.resize(n);
	for (s = 1 ; s <= sn ; ++ s)
	{
		mh = m;
		m *= 2;
		TiComplex wm(cos(PI2 / m), sin(PI2 / m));
		for (k = 0 ; k < n ; k += m)
		{
			TiComplex w(1);
			for (j = 0 ; j < mh; ++ j)
			{
				TiComplex t = w * a[k + j + mh];
				TiComplex u = a[k + j];
				y[k + j] = u + t;
				y[k + j + mh] = u - t;
				w = w * wm;
			}
		}
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
	printf("fft0 time is %lld.\n", t_end - t_start);
	t_start = timeGetTime();
	for (int i = 0; i < times; ++i)
	{
		y = iteration_fft(a);
	}
	t_end = timeGetTime();
	printf("fft1 time is %lld.\n", t_end - t_start);
}

void do_fft_test()
{
	int size = 8;
	// prepare src array
	vec a;
	a.clear();
	for (int i = 0 ; i < size; ++ i)
	{
		TiComplex c(i + 1);
		a.push_back(c);
	}
	// prepare butterfly help array
	bf.clear();
	for (int k = 0 ; k < size ; ++ k)
	{
		bf.push_back(rev_bit(k, size));
	}

	vec y_dft = dft(a);
	vec y_fft0 = recursive_fft(a);
	vec y_fft1 = iteration_fft(a);

	print_vec(y_dft);
	print_vec(y_fft0);
	print_vec(y_fft1);

	time_test(a);
}