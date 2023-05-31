#include "NXSerializable.h"

using namespace rapidjson;

void NXSerializer::StartObject()
{
	m_writer.StartObject();
}

void NXSerializer::EndObject()
{
	m_writer.EndObject();
}

void NXSerializer::String(const std::string& key, const std::string& value)
{
	m_writer.Key(key.c_str());
	m_writer.String(value.c_str());
}

void NXSerializer::Bool(const std::string& key, bool value)
{
	m_writer.Key(key.c_str());
	m_writer.Bool(value);
}

void NXSerializer::Uint64(const std::string& key, size_t value)
{
	m_writer.Key(key.c_str());
	m_writer.Uint64(value);
}

void NXSerializer::Int(const std::string& key, int value)
{
	m_writer.Key(key.c_str());
	m_writer.Int(value);
}

void NXSerializer::SaveToFile(const std::filesystem::path& path)
{
	std::ofstream ofs(path);
	ofs << m_stringBuffer.GetString();
	ofs.close();
}

std::string NXDeserializer::String(const std::string& key)
{
	return m_reader[key.c_str()].GetString();
}

bool NXDeserializer::Bool(const std::string& key)
{
	return m_reader[key.c_str()].GetBool();
}

size_t NXDeserializer::Uint64(const std::string& key)
{
	return m_reader[key.c_str()].GetUint64();
}

int NXDeserializer::Int(const std::string& key)
{
	return m_reader[key.c_str()].GetInt();
}

bool NXDeserializer::LoadFromFile(const std::filesystem::path& path)
{
	std::ifstream ifs(path);
	std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	ifs.close();

	m_reader.Parse(json.c_str());

	return !json.empty();
}
