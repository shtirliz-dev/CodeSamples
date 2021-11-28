#include "Reflection/Function.h"
#include "Reflection/FunctionWrap.h"
#include "Reflection/MetaMethod.h"
#include "Reflection/MetaData.h"

VMetaMethod::VMetaMethod(VFunction* pMethod, const std::unordered_map<std::string, Any>& pAttributes)
{
	mMethod = pMethod;
	mName = pMethod->GetName();
	mMetaData = pMethod->GetReturnType();
	mType = VClassMemberType_Method;
	mAttributes = pAttributes;
}

Any VMetaMethod::Get(const Any& pObject) const
{
	return new VBoxedClassMethod(pObject, this);
}

VFunction* VMetaMethod::Method() const
{
	return mMethod;
}

VBoxedClassMethod::VBoxedClassMethod(const Any& pParent, const VMetaMethod* pMethod) :
	mParent(pParent),
	mMethod(pMethod),
	mFunction(pMethod->Method()),
	VBoxedValue(pMethod->MetaData())
{
}

VBoxedValuePtr VBoxedClassMethod::Invoke()
{
	return mFunction->Invoke(mParent.GetValue()->GetData(mFunction->GetParentType()));
}

VBoxedValuePtr VBoxedClassMethod::Invoke(const std::vector<Any>& pArgs)
{
	return mFunction->Invoke(mParent.GetValue()->GetData(mFunction->GetParentType()), pArgs);
}

const VMetaMethod* VBoxedClassMethod::GetMethod() const
{
	return mMethod;
}
