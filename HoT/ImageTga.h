
#pragma once

struct TiImage
{
	int w;
	int h;
	float* data;
	float* imagi;
};
void loadImageTga(const char* fn, TiImage& image);
bool saveToImage(const char* filename, TiImage& image);