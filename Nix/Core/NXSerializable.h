#include "header.h"

#define NXSERIALIZABLE_DERIVED() \
public: \
	virtual void Serialize() override; \
	virtual void Deserialize() override; \

// 2023.5.30
// Nix ���л����࣬���ڴ����������κ���Ҫ���л����ʲ�
// �̳д��༴�ɽ������л�
class NXSerializable
{
public:
	NXSerializable() {};
	virtual ~NXSerializable() {};

	virtual void Serialize() = 0;
	virtual void Deserialize() = 0;
};
