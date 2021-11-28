#pragma once

#include <unordered_map>
#include <Reflection/Any.h>

class VMetaData;

enum VClassMemberType
{
	VClassMemberType_Field,
	VClassMemberType_Property,
	VClassMemberType_Method,
	VClassMemberType_None
};

class VClassMember
{
protected:
	std::string mName;
	std::unordered_map<std::string, Any> mAttributes;
	VMetaData* mMetaData = nullptr;
	VClassMemberType mType = VClassMemberType_None;

public:
	VMetaData* MetaData() const;
	const std::string& Name() const;
	VClassMemberType Type() const;
	bool HasAttribute(const std::string& pAttribute) const;
	const Any& GetAttribute(const std::string& pAttribute) const;
	virtual Any Get(const Any& pObject) const;
};