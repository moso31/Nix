#include "NXSSSDiffuseProfile.h"

void NXSSSDiffuseProfile::Serialize()
{
	using namespace rapidjson;

	if (!std::filesystem::exists(m_filePath.string()))
	{
		printf("Warning, %s couldn't be serialized, cause path %s does not exist.\n", m_filePath.string().c_str(), m_filePath.string().c_str());
		return;
	}

	NXSerializer serializer;
	serializer.StartObject();
	serializer.Vector3("scatter", m_scatter);
	serializer.Float("scatterDistance", m_scatterDistance);
	serializer.Vector3("transmit", m_transmit);
	serializer.Float("transmitStrength", m_transmitStrength);
	serializer.EndObject();

	serializer.SaveToFile(m_filePath);
}

void NXSSSDiffuseProfile::Deserialize()
{
	using namespace rapidjson;
	if (!std::filesystem::exists(m_filePath.string()))
	{
		printf("Warning, %s couldn't be deserialized, cause path %s does not exist.\n", m_filePath.string().c_str(), m_filePath.string().c_str());
		return;
	}

	NXDeserializer deserializer;
	if (!deserializer.LoadFromFile(m_filePath))
		return;

	m_scatter = deserializer.Vector3("scatter");
	m_scatterDistance = deserializer.Float("scatterDistance");
	m_transmit = deserializer.Vector3("transmit");
	m_transmitStrength = deserializer.Float("transmitStrength");
}
