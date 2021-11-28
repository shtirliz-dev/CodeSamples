#pragma once

#include <Reflection/BoxedValue.h>
#include <Reflection/ClassMember.h>

class VMetaMethod;

class VBoxedClassMethod : public VBoxedValue
{
private:
	Any mParent;
	const VMetaMethod* mMethod;
	VFunction* mFunction;

public:
	VBoxedClassMethod(const Any& pParent, const VMetaMethod* pMethod);

	VBoxedValuePtr Invoke() override;
	VBoxedValuePtr Invoke(const std::vector<Any>& pArgs) override;

	const VMetaMethod* GetMethod() const;
};

class VMetaMethod : public VClassMember
{
private:
	VFunction * mMethod;

public:
	VMetaMethod(VFunction* pMethod, const std::unordered_map<std::string, Any>& pAttributes);

	Any Get(const Any& pObject) const override;

	VFunction* Method() const;
};