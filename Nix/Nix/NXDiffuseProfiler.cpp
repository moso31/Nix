#include "NXDiffuseProfiler.h"

void NXSSSDiffuseProfiler::Serialize()
{
	using namespace rapidjson;

	if (m_filePath.empty())
	{
		printf("Warning, %s couldn't be serialized, cause path %s does not exist.\n", m_filePath.string().c_str(), m_filePath.string().c_str());
		return;
	}

	std::string nxInfoPath = m_filePath.string() + ".n0";

	NXSerializer serializer;
	serializer.StartObject();
	serializer.Vector3("scatter", m_scatter);
	serializer.Float("scatterStrength", m_scatterStrength);
	serializer.Vector3("transmit", m_transmit);
	serializer.Float("transmitStrength", m_transmitStrength);
	serializer.EndObject();

	serializer.SaveToFile(nxInfoPath.c_str());
}

void NXSSSDiffuseProfiler::Deserialize()
{
	using namespace rapidjson;
	if (m_filePath.empty())
	{
		printf("Warning, %s couldn't be deserialized, cause path %s does not exist.\n", m_filePath.string().c_str(), m_filePath.string().c_str());
		return;
	}

	std::string nxInfoPath = m_filePath.string() + ".n0";

	NXDeserializer deserializer;
	if (!deserializer.LoadFromFile(nxInfoPath.c_str()))
		return;

	deserializer.Vector3("scatter", m_scatter);
	deserializer.Float("scatterStrength", m_scatterStrength);
	deserializer.Vector3("transmit", m_transmit);
	deserializer.Float("transmitStrength", m_transmitStrength);
}
