#include "ClassMember.h"
#include "Core/Exception.h"

VMetaData* VClassMember::MetaData() const
{
	return mMetaData;
}

const std::string& VClassMember::Name() const
{
	return mName;
}

VClassMemberType VClassMember::Type() const
{
	return mType;
}

bool VClassMember::HasAttribute(const std::string& pAttribute) const
{
	return mAttributes.find(pAttribute) != mAttributes.end();
}

const Any& VClassMember::GetAttribute(const std::string& pAttribute) const
{
	const auto result = mAttributes.find(pAttribute);

	if (result != mAttributes.end())
	{
		return result->second;
	}

	THROW("Type " + mName + " has no attribute with name: " + pAttribute);
}

Any VClassMember::Get(const Any& pObject) const
{
	return nullptr;
}