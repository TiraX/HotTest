#pragma once

#ifndef _TI_FFT_H__
#define _TI_FFT_H__


#include <vector>
#include <map>
#include <cassert>
#include <algorithm>


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
	TiComplex(float a, float b)
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

	float real() const
	{
		return A;
	}

	float imag() const
	{
		return B;
	}

public:
	float A, B;
};
typedef std::vector<TiComplex> vec;

inline TiComplex operator + (const TiComplex& a, const TiComplex& b)
{
	TiComplex r;
	r.A = a.A + b.A;
	r.B = a.B + b.B;
	return r;
}

inline TiComplex operator - (const TiComplex& a, const TiComplex& b)
{
	TiComplex r;
	r.A = a.A - b.A;
	r.B = a.B - b.B;
	return r;
}

inline TiComplex operator * (const TiComplex& a, const TiComplex& b)
{
	TiComplex r;
	r.A = a.A * b.A - a.B * b.B;
	r.B = a.A * b.B + b.A * a.B;
	return r;
}

inline TiComplex cexp(const TiComplex& c)
{
	assert(c.A == 0);
	return TiComplex(cos(c.B), sin(c.B));
}
inline TiComplex conj(const TiComplex& c)
{
	return TiComplex(c.A, -c.B);
}

class FFT2D;

template <typename T>
class TiMatrix
{
public:
	std::vector<T> Data;
	friend class FFT2D;
	int M, N;
	void resize(int m, int n)
	{
		M = m;
		N = n;
		Data.resize(m * n);
	}

	const T& operator () (int m, int n) const
	{
		return Data[n * M + m];
	}

	T& operator () (int m, int n)
	{
		return Data[n * M + m];
	}

	const T* data() const
	{
		return Data.data();
	}

	T* data()
	{
		return Data.data();
	}

	const T* row_data(int r) const
	{
		return Data.data() + r * M;
	}

	T* row_data(int r)
	{
		return Data.data() + r * M;
	}

	int rows() const
	{
		return N;
	}

	int cols() const
	{
		return M;
	}
};

inline int twiddle_key(short n, short N)
{
	return ((N << 16) & 0xffff) | (n & 0xffff);
}

class FFT2D
{
public:
	int M;
	float* output_array;

	FFT2D()
		: M(0), output_array(nullptr)
	{}

	static std::vector<int> bf;
	static std::map<int, TiComplex> twiddleFactor;

	static int rev_bit(int k, int n)
	{
		int k0 = 0, m = log((double)n) / log((double)2);
		for (int j = 0; j < m; ++j)
		{
			if (k&(1 << j))k0 += (1 << (m - 1 - j));
		}
		return k0;
	}

	FFT2D(int m, float* _out)
		: M(m), output_array(_out)
	{
	}

	void DoFFT(const TiMatrix<TiComplex>& input)
	{
		assert(input.M == M && input.N == N);
		// prepare reverse map
		if (bf.size() != M)
		{
			bf.clear();
			for (int k = 0; k < M; ++k)
			{
				bf.push_back(rev_bit(k, M));
			}
		}
		if (twiddleFactor.size() == 0)
		{
			int p = log2(M);
			int Bp = 1;
			int Np = 1 << p;
			for (int P = 0; P < p; ++P)
			{
				int Np1 = Np >> 1;
				for (int b = 0; b < Bp; ++b)
				{
					for (int n = 0; n < Np1; ++n)
					{
						twiddleFactor[twiddle_key(n, Np)] = TiComplex(cos(-PI * 2.f * n / Np), sin(-PI * 2.f * n / Np));
					}
				}
				Bp <<= 1;
				Np >>= 1;
			}
		}

		// do ifft for input_array
		// cols first
		TiMatrix<TiComplex> result;
		result.resize(M, M);

		vec col;
		col.resize(M);

		const float inv_m = 1.f / M;

		// columns first
		for (int c = 0; c < M; c++)
		{
			int r;
			// collect columns
			for (r = 0; r <= M / 2; r++)
			{
				col[r] = input(c, r);
			}
			for ( ; r < M; r ++)
			{
				col[r] = conj(input(c, M - r));
			}
			// do ifft
			iteration_ifft_dif(col);

			// put into result colums and do reverse copy
			for (r = 0; r < M; r++)
			{
				result(c, r) = col[bf[r]] * inv_m;
			}
		}

		// then rows
		vec row;
		row.resize(M);
		for (int r = 0; r < M; r++)
		{
			// collect rows
			memcpy(row.data(), result.row_data(r), M * sizeof(TiComplex));
			// do ifft
			iteration_ifft_dif(row);

			// put into result rows and do reverse copy
			for (int c = 0; c < M; c++)
			{
				result(c, r) = row[bf[c]] * inv_m;
			}
		}

		// copy to result array
		int index = 0;
		TiComplex* d = result.data();
		for (int r = 0; r < M; r++)
		{
			for (int c = 0; c < M; c++)
			{
				output_array[index] = d[index].A;
				++index;
			}
		}
	}

private:
	void iteration_ifft_dif(vec& src)
	{
		int l = src.size();
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
					TiComplex T(cos(-PI * 2.f * n / Np), sin(-PI * 2.f * n / Np));
					TiComplex e = src[BaseE + n] + src[BaseO + n];
					TiComplex o = (src[BaseE + n] - src[BaseO + n]) * T;
					src[BaseE + n] = e;
					src[BaseO + n] = o;
				}
				BaseE += Np;
			}
			Bp <<= 1;
			Np >>= 1;
		}
		//a.resize(l);
		//bitReverseCopy(a1, a);

		//const float inv_n = 1.f / l;
		//for (int k = 0; k < l; k++)
		//{
		//	a[k] *= inv_n;
		//}
	}
	void iteration_ifft_dit(const vec& y1, vec& y)
	{
		//y.resize(y1.size());
		//bitReverseCopy(y1, y);

		//int n = y.size();
		//int s, m, k, j, mh;
		//int sn = log2(n);
		//m = 1;
		//for (s = 1; s <= sn; ++s)
		//{
		//	mh = m;
		//	m *= 2;
		//	TiComplex wm(cos(-PI2 / m), sin(-PI2 / m));
		//	for (k = 0; k < n; k += m)
		//	{
		//		TiComplex w(1);
		//		for (j = 0; j < mh; ++j)
		//		{
		//			TiComplex t = w * y[k + j + mh];
		//			TiComplex u = y[k + j];
		//			y[k + j] = u + t;
		//			y[k + j + mh] = u - t;
		//			w = w * wm;
		//		}
		//	}
		//}

		//const float inv_n = 1.f / n;
		//for (k = 0; k < n; k++)
		//{
		//	y[k] *= inv_n;
		//}
	}
};

#endif // _TI_FFT_H__
