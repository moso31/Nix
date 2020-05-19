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
};  // ����λͼ����

typedef struct 
{
	WORD    bfType;	//�ļ����ͣ�������0x424D,���ַ���BM��
	DWORD   bfSize;	//�ļ���С
	WORD    bfReserved1; //������
	WORD    bfReserved2; //������
	DWORD   bfOffBits; //���ļ�ͷ��ʵ��λͼ���ݵ�ƫ���ֽ���
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
	DWORD      biSize; //��Ϣͷ��С
	LONG       biWidth; //ͼ����
	LONG       biHeight; //ͼ��߶�
	WORD       biPlanes; //λƽ����������Ϊ1
	WORD       biBitCount; //ÿ����λ��
	DWORD      biCompression; //ѹ������
	DWORD      biSizeImage; //ѹ��ͼ���С�ֽ���
	LONG       biXPelsPerMeter; //ˮƽ�ֱ���
	LONG       biYPelsPerMeter; //��ֱ�ֱ���
	DWORD      biClrUsed; //λͼʵ���õ���ɫ����
	DWORD      biClrImportant; //��λͼ����Ҫ��ɫ����
} BMPINFOHEADER_T; //λͼ��Ϣͷ����

class ImageGenerator
{
public:
	ImageGenerator() {}
	~ImageGenerator() {}

	static void GenerateImageBMP(BYTE * pData, int width, int height, const char * filename);

private:
};