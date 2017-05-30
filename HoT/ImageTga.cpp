/*
Copyright (C) 2012
By Zhao Shuai tirax.cn@gmail.com 2012.9.04
*/

#include "stdafx.h"
#include "ImageTga.h"
#include <fstream>

	// byte-align structures
#if defined(_MSC_VER) 
#	pragma pack( push, packing )
#	pragma pack( 1 )
#	pragma warning(disable:4103)
#elif defined(__ARMCC_VERSION) //RVCT compiler
#	pragma push
#   pragma pack( 1 )
#elif defined(TI_PLATFORM_IOS)
#   pragma push
#   pragma pack( 1 )
#endif

typedef char c8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

	struct STGAHeader
	{
		u8 IdLength;
		u8 ColorMapType;
		u8 ImageType;
		u8 FirstEntryIndex[2];
		u16 ColorMapLength;
		u8 ColorMapEntrySize;
		u8 XOrigin[2];
		u8 YOrigin[2];
		u16 ImageWidth;
		u16 ImageHeight;
		u8 PixelDepth;
		u8 ImageDescriptor;
	};

	struct STGAFooter
	{
		u32 ExtensionOffset;
		u32 DeveloperOffset;
		c8  Signature[18];
	};

	// Default alignment
#if defined(_MSC_VER)
#	pragma pack( pop, packing )
#elif defined(__ARMCC_VERSION) //RVCT compiler
#	pragma pop
#elif defined(TI_PLATFORM_IOS) //RVCT compiler
#	pragma pop
#endif

	using namespace std;
	void loadImageTga(const char* fn, TiImage& image)
	{
		STGAHeader header;

		ifstream f(fn, ios::in | ios::binary);
		
		f.read((char*)&header, sizeof(STGAHeader));

		// skip image identification field
		if (header.IdLength)
			f.seekg(header.IdLength, ios::cur);


		if (header.ColorMapType)
		{
			// skip color map
			f.seekg(header.ColorMapEntrySize / 8 * header.ColorMapLength, ios::cur);
		}

		image.w = header.ImageWidth;
		image.h = header.ImageHeight;

		u8* data = nullptr;
		const int imageSize = header.ImageHeight * header.ImageWidth * header.PixelDepth / 8;
		data = new u8[imageSize];
		f.read((c8*)data, imageSize);

		image.data = new float[header.ImageWidth * header.ImageHeight];
		for (int y = 0; y < header.ImageHeight; y++)
		{
			for (int x = 0; x < header.ImageWidth; x++)
			{
				image.data[y * header.ImageWidth + x] = data[(y * header.ImageWidth + x) * header.PixelDepth / 8] / 255.f;
			}
		}

		delete[] data;
	}

	bool saveToImage(const char* filename, TiImage& image)
	{
		ofstream f(filename, ios::binary);

		STGAHeader header;
		memset(&header, 0, sizeof(STGAHeader));
		header.ImageType = 2;
		header.ImageWidth = image.w;
		header.ImageHeight = image.h;
		header.PixelDepth = 24;
		header.ImageDescriptor = 8;

		f.write((c8*)&header, sizeof(STGAHeader));

		const int image_size = image.w * image.h;

		// convert RGBA to BGRA
		u8* pixels = new u8[image.w * image.h * 3];
		for (int y = 0; y < image.h; y++)
		{
			for (int x = 0; x < image.w; x++)
			{
				u8 v = u8(image.data[y * image.w + x] * 255.f);
				if (v > 255)
				{
					v = 255;
				}
				pixels[(y * image.w + x) * 3 + 0] = v;
				pixels[(y * image.w + x) * 3 + 1] = v;
				pixels[(y * image.w + x) * 3 + 2] = v;
			}
		}

		f.write((c8*)pixels, image.w * image.h * 3);
		delete[] pixels;
		f.close();

		return true;
	}
