// 2023.5.30 by moso31
// Nix 序列化类，基于 rapidjson 库开发，
// 用于处理引擎中任何需要序列化的资产。

#pragma once
#include <filesystem>
#include <fstream>

#if defined(DEBUG) | defined(_DEBUG)
#undef DEBUG_NEW
#undef new
#endif

#include "rapidjson/prettywriter.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"

#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW 
#endif

#include "BaseDefs/Math.h"

class NXSerializer
{
public:
	NXSerializer() : m_writer(m_stringBuffer) {}
	virtual ~NXSerializer() {};

	void StartObject();
	void EndObject();

	void StartArray(const std::string& key);
	void EndArray();
	void PushInt(int value);
	void PushFloat(float value);

	void String(const std::string& key, const std::string& value);
	void Bool(const std::string& key, bool value);
	void Uint64(const std::string& key, size_t value);
	void Uint(const std::string& key, unsigned int value);
	void Int(const std::string& key, int value);
	void Float(const std::string& key, float value);
	void Vector2(const std::string& key, const DirectX::SimpleMath::Vector2& value);
	void Vector3(const std::string& key, const DirectX::SimpleMath::Vector3& value);
	void Vector4(const std::string& key, const DirectX::SimpleMath::Vector4& value);

	std::string Json() const { return m_stringBuffer.GetString(); }

	// 将 Json 写入本地文件
	void SaveToFile(const std::filesystem::path& path);

private:
	rapidjson::StringBuffer m_stringBuffer;
	//rapidjson::Writer<StringBuffer> m_writer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> m_writer;
};

class NXDeserializer
{
public:
	NXDeserializer() {};
	virtual ~NXDeserializer() {};

	std::string String(const std::string& key, const std::string& defaultValue = "");
	bool Bool(const std::string& key, const bool defaultValue = false);
	size_t Uint64(const std::string& key, const size_t defaultValue = 0);
	uint32_t Uint(const std::string& key, const uint32_t defaultValue = 0);
	int Int(const std::string& key, const int defaultValue = 0);
	float Float(const std::string& key, const float defaultValue = 0.0f);
	DirectX::SimpleMath::Vector2 Vector2(const std::string& key, const DirectX::SimpleMath::Vector2 defaultValue = DirectX::SimpleMath::Vector2(0.0f));
	DirectX::SimpleMath::Vector3 Vector3(const std::string& key, const DirectX::SimpleMath::Vector3 defaultValue = DirectX::SimpleMath::Vector3(0.0f));
	DirectX::SimpleMath::Vector4 Vector4(const std::string& key, const DirectX::SimpleMath::Vector4 defaultValue = DirectX::SimpleMath::Vector4(0.0f));

	std::string String(const rapidjson::Value& parent, const std::string& key, const std::string& defaultValue = "");
	bool Bool(const rapidjson::Value& parent, const std::string& key, const bool defaultValue = false);
	size_t Uint64(const rapidjson::Value& parent, const std::string& key, const size_t defaultValue = 0);
	uint32_t Uint(const rapidjson::Value& parent, const std::string& key, const uint32_t defaultValue = 0);
	int Int(const rapidjson::Value& parent, const std::string& key, const int defaultValue = 0);
	float Float(const rapidjson::Value& parent, const std::string& key, const float defaultValue = 0.0f);
	DirectX::SimpleMath::Vector2 Vector2(const rapidjson::Value& parent, const std::string& key, const DirectX::SimpleMath::Vector2 defaultValue = DirectX::SimpleMath::Vector2(0.0f));
	DirectX::SimpleMath::Vector3 Vector3(const rapidjson::Value& parent, const std::string& key, const DirectX::SimpleMath::Vector3 defaultValue = DirectX::SimpleMath::Vector3(0.0f));
	DirectX::SimpleMath::Vector4 Vector4(const rapidjson::Value& parent, const std::string& key, const DirectX::SimpleMath::Vector4 defaultValue = DirectX::SimpleMath::Vector4(0.0f));

	const rapidjson::GenericObject<false, rapidjson::Value> Object(const std::string& key);
	const rapidjson::GenericArray<false, rapidjson::Value> Array(const std::string& key);
	const rapidjson::GenericObject<true, rapidjson::Value> Object(const rapidjson::Value& parent, const std::string& key);
	const rapidjson::GenericArray<true, rapidjson::Value> Array(const rapidjson::Value& parent, const std::string& key);

	// 从本地文件读取 Json
	bool LoadFromFile(const std::filesystem::path& path);

private:
	rapidjson::Document m_reader;
};

// Nix 序列化基类
// 任何需要序列化的类都需要继承此类
class NXSerializable
{
public:
	NXSerializable() = default;
	virtual ~NXSerializable() {};

	virtual void Serialize() = 0;
	virtual void Deserialize() = 0;
};
