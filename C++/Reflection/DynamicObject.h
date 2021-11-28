#pragma once

#include <Reflection/Object.h>
#include <Reflection/Any.h>
#include <Reflection/ScriptingEngine.h>

typedef std::map<std::string, Any> StringAnyMap;
typedef std::unordered_map<std::string, Any> StringAnyUMap;

class VDynamicObject : public VObject
{
	MANAGED_TYPE(VDynamicObject)

private:
	StringAnyUMap mFields;

public:
	void SetField(const std::string& pName, const Any& pValue);
	Any GetField(const std::string& pName) const;
	bool HasField(const std::string& pName) const;

	std::vector<std::string> GetFieldNames() const;
	size_t GetFieldsCount() const;

	bool Empty() const;

	static void ReflectType(VMetaData& pMeta);
};

class VDynamicObjectProtocol
{
public:
	ptr<VDynamicObject> DynamicObject;

	VDynamicObjectProtocol();
	virtual ~VDynamicObjectProtocol();

	static void ReflectType(VMetaData& pMeta);
};
