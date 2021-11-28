#pragma once

#include <Reflection/MetaData.h>

class VTypeManager
{
private:
	std::unordered_map<std::string, VMetaData*> mTypes;
	std::unordered_map<int, VMetaData*> mTypesByIndex;
	
	void AddType(VMetaData* pMetaData);
	bool IsTypeRegistered(VMetaData* pMetaData) const;

public:
	VTypeManager();
	~VTypeManager();

	void RegisterType(VMetaData* pType);
	std::string GetTypeName(int pId);
	int GetTypeId(const std::string& pName);

	void ReflectTypes();

	const VMetaData* GetMetaData(const int pTypeIndex) const;
	const VMetaData* GetMetaData(const std::string& pTypeName) const;
	
	std::string ToString(const Any& pWhat);

	bool CanConvert(const VMetaData* from, const VMetaData* to);
	VBoxedValuePtr Convert(const Any& what, const VMetaData* to);

	int GetTypesCount() const;
	void EnumerateTypes(std::function<void(const VMetaData*)> pEnumCallback) const;

	std::string SerializeJson(const Any& pWhat);
	VBoxedValuePtr DeserializeJson(const std::string& pJson);

public:
	static VTypeManager& Instance();
};