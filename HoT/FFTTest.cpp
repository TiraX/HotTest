// HoT.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FFTTest.h"
#include "ImageTga.h"

#include <vector>
#include <map>
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

vector<int> bf_r2, bf_r4;
map<float, TiComplex> exp_map;

int rev_bit(int k, int n)
{
	int k0 = 0, m = log((double)n) / log((double)2);
	for (int j = 0; j < m; ++j)
	{
		if (k&(1 << j))k0 += (1 << (m - 1 - j));
	}
	return k0;
}


#define MAXPOW 24

int pow_2[MAXPOW];
int pow_4[MAXPOW];

void b4_reorder_map(std::vector<int>& m)
{
	int N = m.size();
	int bits, i, j, k;

	for (i = 0; i<MAXPOW; i++)
		if (pow_2[i] == N)
		{
			bits = i;
			break;
		}

	for (i = 0; i<N; i++)
	{
		j = 0;
		for (k = 0; k<bits; k += 2)
		{
			if (i&pow_2[k]) j += pow_2[bits - k - 2];
			if (i&pow_2[k + 1]) j += pow_2[bits - k - 1];
		}

		if (j>i)  /** Only make "up" swaps */
		{
			m[i] = j;
			m[j] = i;
		}
	}
}
void bitReverseCopyR4(const vec& src, vec &dest)
{
	int n = src.size();
	for (int k = 0; k < n; ++k)
	{
		if (bf_r4[k] == 0)
		{
			dest[k] = src[k];
		}
		else
		{
			dest[bf_r4[k]] = src[k];
		}
	}
}

void bit_r4_reorder(vec& W)
{
	int N = W.size();
	int bits, i, j, k;
	float tempr, tempi;

	for (i = 0; i<MAXPOW; i++)
		if (pow_2[i] == N) bits = i;

	for (i = 0; i<N; i++)
	{
		j = 0;
		for (k = 0; k<bits; k += 2)
		{
			if (i&pow_2[k]) j += pow_2[bits - k - 2];
			if (i&pow_2[k + 1]) j += pow_2[bits - k - 1];
		}

		if (j>i)  /** Only make "up" swaps */
		{
			tempr = W[i].A;
			tempi = W[i].B;
			W[i].A = W[j].A;
			W[i].B = W[j].B;
			W[j].A = tempr;
			W[j].B = tempi;
		}
	}
}
void bitReverseCopy(const vec& src, vec &dest)
{
	int n = src.size();
	for (int k = 0; k <= n - 1; ++k) dest[bf_r2[k]] = src[k];
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

void complex_to_real(const vec& c, vec& r1, vec& r2)
{
	int n = c.size();

	r1.resize(n);
	r2.resize(n);

	r1[0] = c[0].A;
	r2[0] = c[0].B;
	for (int i = 1; i < n; i++)
	{
		r1[i] = TiComplex((c[i].A + c[n - i].A) / 2, (c[i].B - c[n - i].B) / 2);
		r2[i] = TiComplex((c[i].B + c[n - i].B) / 2, (c[n - i].A - c[i].A) / 2);
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


void iteration_fft_dif_r4(vec& a1, vec& a)
{
	int l = a1.size();
	int p = log2(l) / 2;
	int Bp = 1;
	int Np = 4 << p;
	for (int P = 0; P < p; ++P)
	{
		int Np2 = Np >> 2;
		int Base0 = 0;

		for (int b = 0; b < Bp; ++b)
		{
			int Base1 = Base0 + Np2;
			int Base2 = Base1 + Np2;
			int Base3 = Base2 + Np2;
			for (int n = 0; n < Np2; ++n)
			{
				TiComplex a, b, c, d;
				int i0, i1, i2, i3;
				i0 = Base0 + n;
				i1 = Base1 + n;
				i2 = Base2 + n;
				i3 = Base3 + n;

				a = a1[Base0 + n] + a1[Base1 + n] + a1[Base2 + n] + a1[Base3 + n];
				b.A = a1[Base0 + n].A + a1[Base1 + n].B - a1[Base2 + n].A - a1[Base3 + n].B;
				b.B = a1[Base0 + n].B - a1[Base1 + n].A - a1[Base2 + n].B + a1[Base3 + n].A;
				c = a1[Base0 + n] - a1[Base1 + n] + a1[Base2 + n] - a1[Base3 + n];
				d.A = a1[Base0 + n].A - a1[Base1 + n].B - a1[Base2 + n].A + a1[Base3 + n].B;
				d.B = a1[Base0 + n].B + a1[Base1 + n].A - a1[Base2 + n].B - a1[Base3 + n].A;

				TiComplex w0(1), w1, w2, w3;

				float t = float(n) / Np;
				w1 = //exp_map[t * 1];
					TiComplex(cos(PI2 * n * 1 / Np), -sin(PI2 * n * 1 / Np));
				w2 = //exp_map[t * 2];
					TiComplex(cos(PI2 * n * 2 / Np), -sin(PI2 * n * 2 / Np));
				w3 = //exp_map[t * 3];
					TiComplex(cos(PI2 * n * 3 / Np), -sin(PI2 * n * 3 / Np));

				a1[Base0 + n] = a * w0;
				a1[Base1 + n] = b * w1;
				a1[Base2 + n] = c * w2;
				a1[Base3 + n] = d * w3;
			}
			Base0 += Np;
		}
		Bp <<= 2;
		Np >>= 2;
	}
	//a.resize(l);
	//bitReverseCopy(a1, a);
}

void twiddle(TiComplex& W, int N, float stuff)
{
	W.A = cos(stuff*2.0*PI / (float)N);
	W.B = -sin(stuff*2.0*PI / (float)N);
}

/** RADIX-4 FFT ALGORITHM */
void radix4(vec& a1, int N, int start)
{
	TiComplex* x = a1.data() + start;
	int    n2, k1, N1, N2;
	TiComplex W, bfly[4];

	N1 = 4;
	N2 = N / 4;

	/** Do 4 Point DFT */
	for (n2 = 0; n2<N2; n2++)
	{
		/** Don't hurt the butterfly */
		bfly[0].A = (x[n2].A + x[N2 + n2].A + x[2 * N2 + n2].A + x[3 * N2 + n2].A);
		bfly[0].B = (x[n2].B + x[N2 + n2].B + x[2 * N2 + n2].B + x[3 * N2 + n2].B);

		bfly[1].A = (x[n2].A + x[N2 + n2].B - x[2 * N2 + n2].A - x[3 * N2 + n2].B);
		bfly[1].B = (x[n2].B - x[N2 + n2].A - x[2 * N2 + n2].B + x[3 * N2 + n2].A);

		bfly[2].A = (x[n2].A - x[N2 + n2].A + x[2 * N2 + n2].A - x[3 * N2 + n2].A);
		bfly[2].B = (x[n2].B - x[N2 + n2].B + x[2 * N2 + n2].B - x[3 * N2 + n2].B);

		bfly[3].A = (x[n2].A - x[N2 + n2].B - x[2 * N2 + n2].A + x[3 * N2 + n2].B);
		bfly[3].B = (x[n2].B + x[N2 + n2].A - x[2 * N2 + n2].B - x[3 * N2 + n2].A);


		/** In-place results */
		for (k1 = 0; k1<N1; k1++)
		{
			twiddle(W, N, (double)k1*(double)n2);
			x[n2 + N2*k1].A = bfly[k1].A*W.A - bfly[k1].B*W.B;
			x[n2 + N2*k1].B = bfly[k1].B*W.A + bfly[k1].A*W.B;
		}
	}

	/** Don't recurse if we're down to one butterfly */
	if (N2 != 1)
		for (k1 = 0; k1<N1; k1++)
		{
			radix4(a1, N2, N2*k1);
		}
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

void time_test_r2_r4(const vec& a)
{
	vec a1 = a;
	const int times = 1000;
	long long t_start, t_end;
	vec y;
	y.resize(a1.size());
	t_start = timeGetTime();
	for (int i = 0; i < times; ++i)
	{
		iteration_fft_dif(a1, y);
	}
	t_end = timeGetTime();
	printf("r2 time is %lld.\n", t_end - t_start);
	t_start = timeGetTime();
	for (int i = 0; i < times; ++i)
	{
		iteration_fft_dif_r4(a1, y);
		bitReverseCopyR4(a1, y);
	}
	t_end = timeGetTime();
	printf("r4 time is %lld.\n", t_end - t_start);
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
	bf_r2.clear();
	for (int k = 0; k < size; ++k)
	{
		bf_r2.push_back(rev_bit(k, size));
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
	pow_2[0] = 1;
	for (int i = 1; i<MAXPOW; i++)
		pow_2[i] = pow_2[i - 1] * 2;
	pow_4[0] = 1;
	for (int i = 1; i<MAXPOW; i++)
		pow_4[i] = pow_4[i - 1] * 4;

	int size = 256;
	// prepare src array
	vec a, b;
	a.clear();
	int i;
	for (i = 0 ; i < size; ++ i)
	{
		TiComplex c(i + 1);
		a.push_back(c);
	}
	b.clear();
	for (; i < size * 2; i++)
	{
		TiComplex c(i + 1);
		b.push_back(c);
	}

	// prepare butterfly reorder array
	bf_r2.clear();
	for (int k = 0; k < size; ++k)
	{
		bf_r2.push_back(rev_bit(k, size));
	}
	bf_r4.resize(size);
	b4_reorder_map(bf_r4);

	// exp map
	exp_map.clear();
	float sizef = float(size);
	for (int i = 0; i < size; i++)
	{
		exp_map[i / sizef] = TiComplex(cos(PI2 * i / size), -sin(PI2 * i / size));
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

	a1 = a;
	vec y_fft_r4;
	iteration_fft_dif_r4(a1, y_fft_r4);
	//radix4(a1, a1.size(), 0);
	print_vec("fft r4", a1);
	//bit_r4_reorder(a1);
	y_fft_r4.resize(a1.size());
	bitReverseCopyR4(a1, y_fft_r4);
	print_vec("radix 4", y_fft_r4);
	time_test_r2_r4(a);

	vec ao, ao1;
	iteration_ifft_dit(y_fft1, ao);
	iteration_ifft_dif(y_fft2, ao1);

	printf("\nifft test.\n");
	print_vec("ifft dit", ao);
	print_vec("ifft dif", ao1);

	printf("\nreal fft test.\n");
	y_fft1.clear();
	y_fft2.clear();
	iteration_fft_dit(a, y_fft1);
	iteration_fft_dit(b, y_fft2);

	print_vec("fft1", y_fft1);
	print_vec("fft2", y_fft2);

	vec r_fft1, r_fft2;
	vec real_c;
	for (int n = 0; n < size; n++)
	{
		TiComplex c12(a[n].A, b[n].A);
		real_c.push_back(c12);
	}
	vec real_fft;
	iteration_fft_dit(real_c, real_fft);
	complex_to_real(real_fft, r_fft1, r_fft2);
	print_vec("real_fft", real_fft);
	print_vec("real_fft1", r_fft1);
	print_vec("real_fft2", r_fft2);

	vec fft21, fft22;
	iteration_fft_dit(r_fft1, fft21);
	iteration_ifft_dit(r_fft1, fft22);
	print_vec("fft 21: ", fft21);
	print_vec("fft 22: ", fft22);


	time_test(a);
}

void ifft_image(const TiImage& src, TiImage& dest)
{
	// prepare butterfly help array
	bf_r2.clear();
	for (int k = 0; k < src.w; ++k)
	{
		bf_r2.push_back(rev_bit(k, src.w));
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