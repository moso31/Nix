#include "NXSSSDiffuseProfile.h"

void NXSSSDiffuseProfile::Serialize()
{
	using namespace rapidjson;

	if (m_filePath.empty())
	{
		printf("Warning, %s couldn't be serialized, cause path %s does not exist.\n", m_filePath.string().c_str(), m_filePath.string().c_str());
		return;
	}

	NXSerializer serializer;
	serializer.StartObject();
	serializer.Float("radius", m_radius);
	serializer.Vector3("scatter", m_scatter);
	serializer.Float("scatterStrength", m_scatterStrength);
	serializer.Vector3("transmit", m_transmit);
	serializer.Float("transmitStrength", m_transmitStrength);
	serializer.EndObject();

	serializer.SaveToFile(m_filePath);
}

void NXSSSDiffuseProfile::Deserialize()
{
	using namespace rapidjson;
	if (m_filePath.empty())
	{
		printf("Warning, %s couldn't be deserialized, cause path %s does not exist.\n", m_filePath.string().c_str(), m_filePath.string().c_str());
		return;
	}

	NXDeserializer deserializer;
	if (!deserializer.LoadFromFile(m_filePath))
		return;

	deserializer.Float("radius", m_radius);
	deserializer.Vector3("scatter", m_scatter);
	deserializer.Float("scatterStrength", m_scatterStrength);
	deserializer.Vector3("transmit", m_transmit);
	deserializer.Float("transmitStrength", m_transmitStrength);
}
