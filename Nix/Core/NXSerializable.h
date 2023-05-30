// 2023.5.30 by moso31
// Nix ���л��࣬���� rapidjson �⿪����
// ���ڴ����������κ���Ҫ���л����ʲ���

#include <filesystem>
#include <fstream>
#include "rapidjson/writer.h"
#include "rapidjson/document.h"

#define NXSERIALIZABLE_DERIVED() \
public: \
	virtual void Serialize() override; \
	virtual void Deserialize() override; \

using namespace rapidjson;

class NXSerializer
{
public:
	NXSerializer() : m_writer(m_stringBuffer) {}
	~NXSerializer() = default;

	void StartObject();
	void EndObject();

	// string
	void String(const std::string& key, const std::string& value);

	// bool
	void Bool(const std::string& key, bool value);

	// uint64
	void Uint64(const std::string& key, size_t value);

	// int
	void Int(const std::string& key, int value);

	std::string Json() const { return m_stringBuffer.GetString(); }

	// �� Json д�뱾���ļ�
	void SaveToFile(const std::filesystem::path& path);

private:
	StringBuffer m_stringBuffer;
	rapidjson::Writer<StringBuffer> m_writer;
};

class NXDeserializer
{
public:
	NXDeserializer() = default;
	~NXDeserializer() = default;

	// string
	std::string String(const std::string& key);

	// bool
	bool Bool(const std::string& key);

	// uint64
	size_t Uint64(const std::string& key);

	// int
	int Int(const std::string& key);

	// �ӱ����ļ���ȡ Json
	void LoadFromFile(const std::filesystem::path& path);

private:
	Document m_reader;
};

// Nix ���л�����
// �κ���Ҫ���л����඼��Ҫ�̳д���
class NXSerializable
{
public:
	NXSerializable() = default;
	virtual ~NXSerializable() {};

	virtual void Serialize() = 0;
	virtual void Deserialize() = 0;
};
