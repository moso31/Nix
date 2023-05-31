// 2023.5.30 by moso31
// Nix ���л��࣬���� rapidjson �⿪����
// ���ڴ����������κ���Ҫ���л����ʲ���

#include <filesystem>
#include <fstream>

#if defined(DEBUG) | defined(_DEBUG)
#undef DEBUG_NEW
#undef new
#endif

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
	NXDeserializer() {};
	~NXDeserializer() {};

	// string
	std::string String(const std::string& key);

	// bool
	bool Bool(const std::string& key);

	// uint64
	size_t Uint64(const std::string& key);

	// int
	int Int(const std::string& key);

	// �ӱ����ļ���ȡ Json
	bool LoadFromFile(const std::filesystem::path& path);

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
