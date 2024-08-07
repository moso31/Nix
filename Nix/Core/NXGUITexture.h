#pragma once
#include <filesystem>
#include "Ntr.h"
#include "NXTextureDefinitions.h"

class NXTexture;
class NXGUITexture
{
public:
	NXGUITexture();
	virtual ~NXGUITexture() {}

	void Render();
	void Release();

	// set preview image.
	void SetImage(const std::filesystem::path& path);

private:
	Ntr<NXTexture> m_pTexImage;
	NXTextureSerializationData m_texData;
};
