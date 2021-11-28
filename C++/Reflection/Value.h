#pragma once

#include <Reflection/BoxedValue.h>

template <typename T>
struct RemQual
{
	static bool IsConst() { return false; };
	static bool IsPointer() { return false; };
	typedef T type;
};

template <typename T>
struct RemQual<const T>
{
	static bool IsConst() { return true; };
	static bool IsPointer() { return false; };
	typedef T type;
};

template <typename T>
struct RemQual<T&>
{
	static bool IsConst() { return false; };
	static bool IsPointer() { return true; };
	typedef T type;
};

template <typename T>
struct RemQual<const T&>
{
	static bool IsConst() { return true; };
	static bool IsPointer() { return true; };
	typedef T type;
};

template <typename T>
struct RemQual<T&&>
{
	static bool IsConst() { return false; };
	static bool IsPointer() { return true; };
	typedef T type;
};

template <typename T>
struct RemQual<const T&&>
{
	static bool IsConst() { return true; };
	static bool IsPointer() { return true; };
	typedef T type;
};

template <typename T>
struct RemQual<T*>
{
	static bool IsConst() { return false; };
	static bool IsPointer() { return true; };
	typedef T type;
};

template <typename T>
struct RemQual<const T*>
{
	static bool IsConst() { return true; };
	static bool IsPointer() { return true; };
	typedef T type;
};

class VDataCastHelper
{
public:
	static RawPtr CastPointer(const VMetaData* pMainMetaData, RawPtr pPointer, const VMetaData* pTargetType);
};

template<typename T>
VMetaData* GetMetaData();

template<typename T, typename U = void>
class Value;

template<typename T>
class Value<T, typename std::enable_if<!std::is_base_of<VObject, T>::value>::type> : public VBoxedValue
{
public:
	typedef typename RemQual<T>::type Data_t;
	typedef Value<Data_t> Value_t;
	typedef ptr<Value<Data_t>> ValuePtr_t;

private:
	Data_t * mData = nullptr;
	bool mDataOwner = false;

public:
	RawPtr GetData(const VMetaData* pTargetType) const override
	{
		return VDataCastHelper::CastPointer(mMetaData, mData, pTargetType);
	}

	void Set(const VBoxedValuePtr& pValue) override
	{
		auto otherMeta = pValue->GetMetaData();

		if (mMetaData == otherMeta)
		{
			*mData = *reinterpret_cast<Data_t*>(pValue->GetData(mMetaData));
		}
	}

	bool IsDataOwner() const override
	{
		return mDataOwner;
	}

	static ValuePtr_t CreateHolder(Data_t* pData)
	{
		if (pData == nullptr)
		{
			return nullptr;
		}

		return new Value_t(pData, ::GetMetaData<Data_t>(), false);
	}

	static ValuePtr_t CreateOwner(Data_t* pData)
	{
		if (pData == nullptr)
		{
			return nullptr;
		}

		return new Value_t(pData, ::GetMetaData<Data_t>(), true);
	}

	static ValuePtr_t Create()
	{
		return new Value_t(new Data_t(), ::GetMetaData<Data_t>(), true);
	}

	template <typename First, typename... Tail>
	static ValuePtr_t Create(First&& pFirst, Tail&&... pTail)
	{
		return new Value_t(new Data_t(pFirst, std::forward<Tail>(pTail)...), ::GetMetaData<Data_t>(), true);
	}

	~Value()
	{
		if (mDataOwner)
		{
			delete mData;
			mData = nullptr;
		}
	}

private:
	Value(Data_t* pData, const VMetaData* pMetaData, bool pOwner) :
		VBoxedValue(pMetaData),
		mData(pData),
		mDataOwner(pOwner)
	{
	}
};

template<typename T>
class Value<T, typename std::enable_if<std::is_base_of<VObject, T>::value>::type> : public VBoxedValue
{
public:
	typedef typename RemQual<T>::type Data_t;
	typedef Value<Data_t> Value_t;
	typedef ptr<Value<Data_t>> ValuePtr_t;

private:
	ptr<T> mData;

public:
	RawPtr GetData(const VMetaData* pTargetType) const override
	{
		return VDataCastHelper::CastPointer(mMetaData, mData.Get(), pTargetType);
	}

	static ValuePtr_t CreateOwner(Data_t* pData)
	{
		return Create(pData);
	}

	static ValuePtr_t CreateHolder(Data_t* pData)
	{
		return Create(pData);
	}

	static ValuePtr_t Create(const Data_t& pObject)
	{
		return Create(const_cast<Data_t*>(&pObject));
	}

	static ValuePtr_t Create(Data_t* pObject)
	{
		if (pObject)
		{
			return new Value_t(pObject, pObject->MetaData());
		}

		return nullptr;
	}

private:
	Value(Data_t* pData, const VMetaData* pMetaData) :
		VBoxedValue(pMetaData),
		mData(pData)
	{
	}
};