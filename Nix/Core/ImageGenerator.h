#pragma once
#include <Windows.h>
#include <pshpack2.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ImageBMPData
{
	BYTE b;
	BYTE g;
	BYTE r;
};  // 定义位图数据

typedef struct 
{
	WORD    bfType;	//文件类型，必须是0x424D,即字符“BM”
	DWORD   bfSize;	//文件大小
	WORD    bfReserved1; //保留字
	WORD    bfReserved2; //保留字
	DWORD   bfOffBits; //从文件头到实际位图数据的偏移字节数
} BMPFILEHEADER_T;


struct BMPFILEHEADER_S 
{
	WORD    bfType;
	DWORD   bfSize;
	WORD    bfReserved1;
	WORD    bfReserved2;
	DWORD   bfOffBits;
};

typedef struct 
{
	DWORD      biSize; //信息头大小
	LONG       biWidth; //图像宽度
	LONG       biHeight; //图像高度
	WORD       biPlanes; //位平面数，必须为1
	WORD       biBitCount; //每像素位数
	DWORD      biCompression; //压缩类型
	DWORD      biSizeImage; //压缩图像大小字节数
	LONG       biXPelsPerMeter; //水平分辨率
	LONG       biYPelsPerMeter; //垂直分辨率
	DWORD      biClrUsed; //位图实际用到的色彩数
	DWORD      biClrImportant; //本位图中重要的色彩数
} BMPINFOHEADER_T; //位图信息头定义

class ImageGenerator
{
public:
	ImageGenerator() {}
	~ImageGenerator() {}

	static void GenerateImageBMP(BYTE * pData, int width, int height, const char * filename);

private:
};