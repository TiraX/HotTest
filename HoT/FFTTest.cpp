// HoT.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FFTTest.h"
#include "ImageTga.h"

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

	TiComplex& operator *= (float o)
	{
		A *= o;
		B *= o;
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
void iteration_fft_dit(const vec& a1, vec& a)
{
	a.resize(a1.size());
	bitReverseCopy(a1, a);

	int n = a.size();
	int s, m, k, j, mh;
	int sn = log2(n);
	m = 1;
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
				a[k + j] = u + t;
				a[k + j + mh] = u - t;
				w = w * wm;
			}
		}
	}
}

void iteration_ifft_dit(const vec& y1, vec& y)
{
	y.resize(y1.size());
	bitReverseCopy(y1, y);

	int n = y.size();
	int s, m, k, j, mh;
	int sn = log2(n);
	m = 1;
	for (s = 1; s <= sn; ++s)
	{
		mh = m;
		m *= 2;
		TiComplex wm(cos(-PI2 / m), sin(-PI2 / m));
		for (k = 0; k < n; k += m)
		{
			TiComplex w(1);
			for (j = 0; j < mh; ++j)
			{
				TiComplex t = w * y[k + j + mh];
				TiComplex u = y[k + j];
				y[k + j] = u + t;
				y[k + j + mh] = u - t;
				w = w * wm;
			}
		}
	}

	const float inv_n = 1.f / n;
	for (k = 0; k < n; k++)
	{
		y[k] *= inv_n;
	}
}

void iteration_fft_dif(vec& a1, vec& a)
{
	int l = a1.size();
	int p = log2(l);
	int Bp = 1;
	int Np = 1 << p;
	for (int P = 0 ; P < p ; ++ P)
	{
		int Np1 = Np >> 1;
		int BaseE = 0;
		for (int b = 0 ; b < Bp ; ++ b)
		{
			int BaseO = BaseE + Np1;
			for (int n = 0 ; n < Np1 ; ++ n)
			{
				TiComplex T(cos(PI2 * n / Np), sin(PI2 * n / Np));
				TiComplex e = a1[BaseE + n] + a1[BaseO + n];
				TiComplex o = (a1[BaseE + n] - a1[BaseO + n]) * T;
				a1[BaseE + n] = e;
				a1[BaseO + n] = o;
			}
			BaseE += Np;
		}
		Bp <<= 1;
		Np >>= 1;
	}
	a.resize(l);
	bitReverseCopy(a1, a);
}

void iteration_ifft_dif(vec& a1, vec& a)
{
	int l = a1.size();
	int p = log2(l);
	int Bp = 1;
	int Np = 1 << p;
	for (int P = 0; P < p; ++P)
	{
		int Np1 = Np >> 1;
		int BaseE = 0;
		for (int b = 0; b < Bp; ++b)
		{
			int BaseO = BaseE + Np1;
			for (int n = 0; n < Np1; ++n)
			{
				TiComplex T(cos(-PI2 * n / Np), sin(-PI2 * n / Np));
				TiComplex e = a1[BaseE + n] + a1[BaseO + n];
				TiComplex o = (a1[BaseE + n] - a1[BaseO + n]) * T;
				a1[BaseE + n] = e;
				a1[BaseO + n] = o;
			}
			BaseE += Np;
		}
		Bp <<= 1;
		Np >>= 1;
	}
	a.resize(l);
	bitReverseCopy(a1, a);

	const float inv_n = 1.f / l;
	for (int k = 0; k < l; k++)
	{
		a[k] *= inv_n;
	}
}


void print_vec(const char* name, const vec& v)
{
	int n = v.size();
	printf("%s, n = %d\n---------------\n", name, n);
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
		iteration_fft_dit(a, y);
	}
	t_end = timeGetTime();
	printf("fft1 time is %lld.\n", t_end - t_start);
}

void test_image()
{
	int size = 4;

	// load image
	TiImage img;
	loadImageTga("test.tga", img);
	size = img.w;

	saveToImage("test_o.tga", img);

	// prepare butterfly help array
	bf.clear();
	for (int k = 0; k < size; ++k)
	{
		bf.push_back(rev_bit(k, size));
	}

	long long t_start = timeGetTime();
	// line fft
	vec *lines = new vec[img.h];
	vec line;
	line.resize(img.w);
	for (int y = 0; y < img.h; y++)
	{
		// gather data
		for (int x = 0; x < img.w; x++)
		{
			TiComplex a(img.data[y * img.w + x]);
			line[x] = a;
		}

		// do fft
		iteration_fft_dit(line, lines[y]);
	}
	long long t_end = timeGetTime();
	printf("line finished %lld.\n", t_end - t_start);

	vec* cols = new vec[img.w];
	t_start = t_end;
	for (int x = 0; x < img.w; x++)
	{
		// gather data
		for (int y = 0; y < img.h; y++)
		{
			line[y] = lines[y][x];
		}

		// do fft
		iteration_fft_dit(line, cols[x]);
	}
	t_end = timeGetTime();
	printf("cols finished %lld.\n", t_end - t_start);

	print_vec("image", cols[0]);

	//cols[0][0].A = 0.f;

	// ifft
	vec* ifft_lines = new vec[img.w];
	for (int y = 0; y < img.h; y++)
	{
		ifft_lines[y].resize(img.w);
	}
	for (int x = 0; x < img.w; x++)
	{
		vec ifft_col;
		iteration_ifft_dit(cols[x], ifft_col);
		for (int y = 0; y < img.h; y++)
		{
			ifft_lines[y][x] = ifft_col[y];
		}
	}
	TiImage newImg;
	newImg.w = img.w;
	newImg.h = img.h;
	newImg.data = new float[img.w * img.h];
	for (int y = 0; y < img.h; y++)
	{
		vec ifft_line;
		iteration_ifft_dit(ifft_lines[y], ifft_line);
		for (int x = 0; x < img.w; x++)
		{
			newImg.data[y * img.w + x] = ifft_line[x].A;
		}
	}

	saveToImage("test_new.tga", newImg);

	// try to output an image
	// copy data
	TiComplex nmax(-99999999999.f, -99999999999.f);
	TiComplex nmin(99999999999.f, 99999999999.f);
	TiImage img_real, img_imagi;

	img_real.w = img_imagi.w = img.w;
	img_real.h = img_imagi.h = img.h;

	img_real.data = new float[img.w * img.h];
	img_imagi.data = new float[img.w * img.h];

	for (int y = 0; y < img.h; y++)
	{
		for (int x = 0; x < img.w; x++)
		{
			const TiComplex& n = cols[x][y];
			img_real.data[y * img.w + x] = n.A;
			img_imagi.data[y * img.w + x] = n.B;

			if (x * y != 0)
			{
				if (n.A > nmax.A)
					nmax.A = n.A;
				if (n.B > nmax.B)
					nmax.B = n.B;
				if (n.A < nmin.A)
					nmin.A = n.A;
				if (n.B < nmin.B)
					nmin.B = n.B;
			}
		}
	}

	// normalize
	float a = 1.f / (nmax.A - nmin.A);
	float b = 1.f / (nmax.B - nmin.B);
	for (int y = 0; y < img.h; y++)
	{
		for (int x = 0; x < img.w; x++)
		{
			int offset = y * img.w + x;
			img_real.data[offset] = (img_real.data[offset] - nmin.A) * a;
			img_imagi.data[offset] = (img_imagi.data[offset] - nmin.B) * b;
		}
	}

	saveToImage("test_real.tga", img_real);
	saveToImage("test_imgi.tga", img_imagi);

}

void test_fft()
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
	for (int k = 0; k < size; ++k)
	{
		bf.push_back(rev_bit(k, size));
	}

	vec y_dft = dft(a);
	vec y_fft0 = recursive_fft(a);
	vec y_fft1;
	iteration_fft_dit(a, y_fft1);
	vec a1 = a;
	vec y_fft2;
	iteration_fft_dif(a1, y_fft2);

	printf("fft test.\n");
	print_vec("dft", y_dft);
	print_vec("fft recursive", y_fft0);
	print_vec("fft dit", y_fft1);
	print_vec("fft dif", y_fft2 );

	vec ao, ao1;
	iteration_ifft_dit(y_fft1, ao);
	iteration_ifft_dif(y_fft2, ao1);

	printf("\nifft test.\n");
	print_vec("ifft dit", ao);
	print_vec("ifft dif", ao1);


	time_test(a);
}

void ifft_image(const TiImage& src, TiImage& dest)
{
	// prepare butterfly help array
	bf.clear();
	for (int k = 0; k < src.w; ++k)
	{
		bf.push_back(rev_bit(k, src.w));
	}
	// ifft
	vec* ifft_lines = new vec[src.h];
	// cols
	vec* ifft_cols = new vec[src.w];
	for (int x = 0; x < src.w; x++)
	{
		ifft_cols[x].resize(src.w);
		ifft_lines[x].resize(src.h);
		for (int y = 0; y < src.h; y++)
		{
			int offset = y * src.w + x;
			ifft_cols[x][y] = TiComplex(src.data[offset], src.imagi[offset] );
		}
	}
	for (int x = 0; x < src.w; x++)
	{
		vec ifft_col;
		iteration_ifft_dit(ifft_cols[x], ifft_col);
		for (int y = 0; y < src.h; y++)
		{
			ifft_lines[y][x] = ifft_col[y];
		}
	}
	// lines
	dest.w = src.w;
	dest.h = src.h;
	dest.data = new float[src.w * src.h];
	for (int y = 0; y < src.h; y++)
	{
		vec line;
		iteration_ifft_dit(ifft_lines[y], line);
		for (int x = 0; x < src.w; x++)
		{
			dest.data[y * src.w + x] = line[x].A;
		}
	}
}

void do_fft_test()
{
	test_fft();

	//test_image();
}