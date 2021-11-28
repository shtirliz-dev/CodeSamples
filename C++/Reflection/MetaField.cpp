
#include "Reflection/MetaField.h"
#include "Reflection/MetaData.h"

VMetaField::VMetaField(const std::string& pName, VMetaData* pMetaData, bool pIsRefOrPointer, size_t pOffset, const std::unordered_map<std::string, Any>& pAttributes)
{
	mIsPointer = pIsRefOrPointer;
	mOffset = pOffset;
	mName = pName;
	mMetaData = pMetaData;
	mType = VClassMemberType_Field;
	mAttributes = pAttributes;
}

size_t VMetaField::Offset() const
{
	return mOffset;
}

bool VMetaField::IsPointer() const
{
	return mIsPointer;
}