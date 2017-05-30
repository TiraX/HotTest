
#pragma once

struct TiImage;

void do_fft_test();

void fft_image(const TiImage& src, TiImage& dest);
void ifft_image(const TiImage& src, TiImage& dest);