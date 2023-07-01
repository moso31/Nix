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

void NXSerializer::StartArray(const std::string& key)
{
	m_writer.Key(key.c_str());
	m_writer.StartArray();
}

void NXSerializer::EndArray()
{
	m_writer.EndArray();
}

void NXSerializer::PushInt(int value)
{
	m_writer.Int(value);
}

void NXSerializer::PushFloat(float value)
{
	m_writer.Double((double)value);
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

void NXSerializer::Uint(const std::string& key, unsigned int value)
{
	m_writer.Key(key.c_str());
	m_writer.Uint(value);
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

std::string NXDeserializer::String(const std::string& key, const std::string& defaultValue)
{
	if (m_reader.HasMember(key.c_str()))
	{
		auto& val = m_reader[key.c_str()];
		if (val.IsString()) return val.GetString();	
	}
	return defaultValue;
}

bool NXDeserializer::Bool(const std::string& key, const bool defaultValue)
{
	if (m_reader.HasMember(key.c_str()))
	{
		auto& val = m_reader[key.c_str()];
		if (val.IsBool()) return val.GetBool();
	}
	return defaultValue;
}

size_t NXDeserializer::Uint64(const std::string& key, const size_t defaultValue)
{
	if (m_reader.HasMember(key.c_str()))
	{
		auto& val = m_reader[key.c_str()];
		if (val.IsUint64()) return val.GetUint64();
	}
	return defaultValue;
}

int NXDeserializer::Int(const std::string& key, const int defaultValue)
{
	if (m_reader.HasMember(key.c_str()))
	{
		auto& val = m_reader[key.c_str()];
		if (val.IsInt()) return val.GetInt();
	}
	return defaultValue;
}

std::string NXDeserializer::String(const rapidjson::Value& parent, const std::string& key, const std::string& defaultValue)
{
	if (parent.HasMember(key.c_str()))
	{
		auto& val = parent[key.c_str()];
		if (val.IsString()) return val.GetString();
	}
	return defaultValue;
}

bool NXDeserializer::Bool(const rapidjson::Value& parent, const std::string& key, const bool defaultValue)
{
	if (parent.HasMember(key.c_str()))
	{
		auto& val = parent[key.c_str()];
		if (val.IsBool()) return val.GetBool();
	}
	return defaultValue;
}

size_t NXDeserializer::Uint64(const rapidjson::Value& parent, const std::string& key, const size_t defaultValue)
{
	if (parent.HasMember(key.c_str()))
	{
		auto& val = parent[key.c_str()];
		if (val.IsUint64()) return val.GetUint64();
	}
	return defaultValue;
}

int NXDeserializer::Int(const rapidjson::Value& parent, const std::string& key, const int defaultValue)
{
	if (parent.HasMember(key.c_str()))
	{
		auto& val = parent[key.c_str()];
		if (val.IsInt()) return val.GetInt();
	}
	return defaultValue;
}

const GenericObject<false, Value> NXDeserializer::Object(const std::string& key)
{
	return m_reader[key.c_str()].GetObject();
}

const GenericArray<false, Value> NXDeserializer::Array(const std::string& key)
{
	return m_reader[key.c_str()].GetArray();
}

const GenericObject<true, Value> NXDeserializer::Object(const rapidjson::Value& parent, const std::string& key)
{
	if (parent.HasMember(key.c_str()))
	{
		auto& val = parent[key.c_str()];
		if (val.IsObject()) return val.GetObject();
	}

	static const Value emptyObject(rapidjson::kObjectType);
	return emptyObject.GetObject();
}

const GenericArray<true, Value> NXDeserializer::Array(const rapidjson::Value& parent, const std::string& key)
{
	if (parent.HasMember(key.c_str()))
	{
		auto& val = parent[key.c_str()];
		if (val.IsArray()) return val.GetArray();
	}
	
	static const Value emptyArray(rapidjson::kArrayType);
	return emptyArray.GetArray();
}

bool NXDeserializer::LoadFromFile(const std::filesystem::path& path)
{
	std::ifstream ifs(path);
	std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	ifs.close();

	m_reader.Parse(json.c_str());

	return !json.empty();
}
