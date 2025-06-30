#pragma once
#include <filesystem>
#include "Ntr.h"
#include "NXTextureDefinitions.h"

struct NXTerrainNormalMapBakeData
{
	float worldSize[2] = { 2048.0f, 2048.0f };
	float heightRange[2] = { 0.0f, 1000.0f };
};

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
	void Render_Texture();
	void Render_RawTexture();
	void Render_TexturePreview();
	void Render_Preview2D();

	void Render_BakePopup();
	void SaveNormalMap();

private:
	std::filesystem::path m_path;
	Ntr<NXTexture> m_pTexImage;
	NXTextureSerializationData m_texData;
	int m_guiImgRawWH[2] = { 1, 1 };

	// �����cube����2DArray������Ԥ������slice
	int m_preview2DArraySliceIndex;

	// ����Ŀǰ�������GUI�決���η���ͼ
	// ������Ҫ�ṩһ�������ȷ���εĻ������ݣ������С�͸߶ȷ�Χ
	// todo��������Ӧ�û�����һ��terrainDataר�Ŵ洢�ȽϺ�
	// todo���������ݵ����˼�뻹ûȷ�������������Ǻ�terrainDataӲ�󶨣�����ֱ����Ƴɲ��ʲ�����
	NXTerrainNormalMapBakeData m_terrainNormalMapBakeData;
};
