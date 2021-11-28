#pragma once

#include <Reflection/MetaData.h>
#include <Reflection/Any.h>

class VMetaProperty : public VClassMember
{
private:
	VFunction* mSetter;
	VFunction* mGetter;

public:
	VMetaProperty(const std::string& pName, 
		VFunction* mGetter, 
		VFunction* mSetter, 
		const std::unordered_map<std::string, Any>& pTags = std::unordered_map<std::string, Any>());

	VFunction* Getter() const;
	VFunction* Setter() const;

	Any Get(const Any& pObject) const;
};

class VBoxedClassProperty : public VBoxedValue
{
private:
	Any mParent;
	const VMetaProperty* mProperty = nullptr;

public:
	VBoxedClassProperty(const Any& pParent, const VMetaProperty* pProperty);

	VBoxedValuePtr Get() override;
	void Set(const VBoxedValuePtr& pValue) override;

	bool IsUnboxable() const override;
};