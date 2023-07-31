// 2023.5.30 by moso31
// Nix 序列化类，基于 rapidjson 库开发，
// 用于处理引擎中任何需要序列化的资产。

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

using namespace rapidjson;

class NXSerializer
{
public:
	NXSerializer() : m_writer(m_stringBuffer) {}
	~NXSerializer() = default;

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

	std::string Json() const { return m_stringBuffer.GetString(); }

	// 将 Json 写入本地文件
	void SaveToFile(const std::filesystem::path& path);

private:
	StringBuffer m_stringBuffer;
	//rapidjson::Writer<StringBuffer> m_writer;
	rapidjson::PrettyWriter<StringBuffer> m_writer;
};

class NXDeserializer
{
public:
	NXDeserializer() {};
	~NXDeserializer() {};

	std::string String(const std::string& key, const std::string& defaultValue = "");
	bool Bool(const std::string& key, const bool defaultValue = false);
	size_t Uint64(const std::string& key, const size_t defaultValue = 0);
	int Int(const std::string& key, const int defaultValue = 0);
	float Float(const std::string& key, const float defaultValue = 0.0f);

	std::string String(const rapidjson::Value& parent, const std::string& key, const std::string& defaultValue = "");
	bool Bool(const rapidjson::Value& parent, const std::string& key, const bool defaultValue = false);
	size_t Uint64(const rapidjson::Value& parent, const std::string& key, const size_t defaultValue = 0);
	int Int(const rapidjson::Value& parent, const std::string& key, const int defaultValue = 0);
	float Float(const rapidjson::Value& parent, const std::string& key, const float defaultValue = 0.0f);

	const GenericObject<false, Value> Object(const std::string& key);
	const GenericArray<false, Value> Array(const std::string& key);

	const GenericObject<true, Value> Object(const rapidjson::Value& parent, const std::string& key);
	const GenericArray<true, Value> Array(const rapidjson::Value& parent, const std::string& key);

	// 从本地文件读取 Json
	bool LoadFromFile(const std::filesystem::path& path);

private:
	Document m_reader;
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
