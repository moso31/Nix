#include "NXConverter.h"
#include "NXTerrainLayer.h"

NXTerrainLayer::NXTerrainLayer(const std::string& name) :
	m_name(name)
{
}

void NXTerrainLayer::Serialize()
{
	if (m_path.empty())
	{
		printf("Warning, %s couldn't be serialized, cause path %s does not exist.\n", m_path.string().c_str(), m_path.string().c_str());
		return;
	}

	NXSerializer serializer;
	std::string nxInfoPath = m_path.string() + ".n0";
	if (NXConvert::IsTerrainLayerExtension(m_path.extension().string()))
	{
		serializer.StartObject();
		serializer.String("path", m_path.string());
		serializer.String("heightMapPath", m_heightMapPath.string());
		serializer.EndObject();

		serializer.SaveToFile(m_path);
	}
}

void NXTerrainLayer::Deserialize()
{
	std::string nxInfoPath = m_path.string() + ".n0";

	NXDeserializer deserializer;
	bool bJsonExist = deserializer.LoadFromFile(nxInfoPath.c_str());
	if (bJsonExist)
	{
	}
	else
	{
	}
}
