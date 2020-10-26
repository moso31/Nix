#pragma once
#include "NXPrimitive.h"
#include "DirectXTex.h"

class NXPBREnvironmentLight;

class NXCubeMap : public NXPrimitive
{
public:
	NXCubeMap(const std::shared_ptr<NXScene>& pScene);
	~NXCubeMap() {}

	bool Init(std::wstring filePath);
	Vector3 BackgroundColorByDirection(const Vector3& v);

	void SetEnvironmentLight(std::shared_ptr<NXPBREnvironmentLight> pEnvironmentLight) { m_pEnvironmentLight = pEnvironmentLight; }
	std::shared_ptr<NXPBREnvironmentLight> GetEnvironmentLight() const;

	void Update() override;
	void Render() override;
	void Release() override;

private:
	void InitVertex();
	void InitVertexIndexBuffer() override;

private:
	std::unique_ptr<ScratchImage> m_image;
	byte* m_faceData[6];
	size_t m_width, m_height;

	std::shared_ptr<NXScene> m_pScene;
	std::shared_ptr<NXPBREnvironmentLight> m_pEnvironmentLight;

	std::vector<VertexP>	m_vertices;
};