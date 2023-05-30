#include "header.h"

#define NXSERIALIZABLE_DERIVED() \
public: \
	virtual void Serialize() override; \
	virtual void Deserialize() override; \

// 2023.5.30
// Nix 序列化基类，用于处理引擎中任何需要序列化的资产
// 继承此类即可进行序列化
class NXSerializable
{
public:
	NXSerializable() {};
	virtual ~NXSerializable() {};

	virtual void Serialize() = 0;
	virtual void Deserialize() = 0;
};
