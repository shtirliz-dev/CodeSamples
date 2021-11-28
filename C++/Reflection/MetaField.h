#pragma once

#include <Reflection/MetaData.h>

class VMetaField : public VClassMember
{
private:
	size_t mOffset;
	bool mIsPointer;
	
public:
	VMetaField(const std::string& pName, VMetaData* pMetaData, bool pIsRefOrPointer, size_t pOffset, const std::unordered_map<std::string, Any>& pAttributes = std::unordered_map<std::string, Any>());
		
	size_t Offset() const;
	bool IsPointer() const;
};