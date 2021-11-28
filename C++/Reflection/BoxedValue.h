#pragma once

#include <Reflection/Object.h>

class VBoxedValue;
class VMetaData;
class Any;

typedef ptr<VBoxedValue> VBoxedValuePtr;

class VBoxedValue : public VObject
{
protected:
	const VMetaData* mMetaData = nullptr;

public:
	VBoxedValue(const VMetaData* pMetaData);

	virtual RawPtr GetData(const VMetaData* pTargetType) const;
	virtual const VMetaData* GetMetaData() const;

	virtual VBoxedValuePtr Invoke();
	virtual VBoxedValuePtr Invoke(const std::vector<Any>& pArgs);

	virtual VBoxedValuePtr Get();
	virtual void Set(const VBoxedValuePtr& pValue);

	virtual bool IsDataOwner() const;
	virtual bool IsUnboxable() const;

	static void* operator new(size_t size);
	static void operator delete(void* data);
};

class VBoxedHolder : public VBoxedValue
{
private:
	RawPtr mData = nullptr;

public:
	VBoxedHolder(RawPtr pData, const VMetaData* pMetaData);

	void Set(const VBoxedValuePtr& pData) override;
	RawPtr GetData(const VMetaData* pTargetType) const override;
};