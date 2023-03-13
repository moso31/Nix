#pragma once
#include "Header.h"

class NXGUITexture
{
public:
	NXGUITexture();
	~NXGUITexture() {}

	void Render();

private:
	// set preview image.
	void SetImage(const std::string& strImgPath) { m_strImgPath = strImgPath; }

private:
	std::string m_strImgPath;
};