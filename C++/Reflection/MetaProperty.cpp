#include "Reflection/Function.h"
#include "Reflection/FunctionWrap.h"
#include "Reflection/MetaProperty.h"
#include "Containers/Array.h"

VMetaProperty::VMetaProperty(const std::string& pName, VFunction* pGetter, VFunction* pSetter, const std::unordered_map<std::string, Any>& pTags)
{
	mGetter = pGetter;
	mSetter = pSetter;
	mType = VClassMemberType_Property;
	mName = pName;
	mMetaData = pGetter->GetReturnType();
	mAttributes = pTags;
}

VFunction* VMetaProperty::Getter() const
{
	return mGetter;
}

VFunction* VMetaProperty::Setter() const
{
	return mSetter;
}

Any VMetaProperty::Get(const Any& pObject) const
{
	return new VBoxedClassProperty(pObject, this);
}

VBoxedClassProperty::VBoxedClassProperty(const Any& pParent, const VMetaProperty* pProperty) :
	mParent(pParent),
	mProperty(pProperty),
	VBoxedValue(pProperty->MetaData())
{
}

bool VBoxedClassProperty::IsUnboxable() const
{
	return true;
}

VBoxedValuePtr VBoxedClassProperty::Get()
{
	if (const auto getter = mProperty->Getter())
	{
		return getter->Invoke(mParent.GetValue()->GetData(getter->GetParentType()));
	}

	throw std::runtime_error("Property " + mProperty->Name() + " is not readable!");
}

void VBoxedClassProperty::Set(const VBoxedValuePtr& pValue)
{
	if (const auto setter = mProperty->Setter())
	{
		setter->Invoke(mParent.GetValue()->GetData(setter->GetParentType()), { pValue });
	}
	else
	{
		throw std::runtime_error("Property " + mProperty->Name() + " is not writeable!");
	}
}