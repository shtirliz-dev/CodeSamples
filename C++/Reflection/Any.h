#pragma once

#include <Reflection/Value.h>
#include <Core/Log.h>

class VFunction;
class Any;

template<typename T, typename... Args>
Any Unpack(const T& what, const Args&... args);

class Any
{
private:
	VBoxedValuePtr mValue;

private:
	Any Invoke() const;
	Any Invoke(const std::vector<Any>& pArgs) const;
	Any GetMember(const std::string& pName) const;
	VBoxedValuePtr Unbox() const;

public:
	Any();
	Any(const char* pString);
	Any(const wchar_t* pString);
	Any(const nullptr_t& pNullPointer);
	Any(const Any& pAny);
	Any(const VBoxedValuePtr& pBox);

	template<typename T, typename std::enable_if<std::is_base_of<VBoxedValue, T>::value>::type* = nullptr>
	Any(const ptr<T>& pObject)
	{
		mValue = pObject.template FastCast<VBoxedValue>();
	}

	template<typename T, typename std::enable_if<!std::is_base_of<VBoxedValue, T>::value>::type* = nullptr>
	Any(const ptr<T>& pObject)
	{
		mValue = Value<T>::Create(pObject.Get());
	}

	template<typename T>
	Any(const T& pObject)
	{
		mValue = Value<T>::Create(pObject);
	}

	template<typename T, typename std::enable_if<std::is_base_of<VBoxedValue, T>::value>::type* = nullptr>
	Any(T* pObject)
	{
		mValue = VBoxedValuePtr(pObject);
	}

	template<typename T, typename std::enable_if<!std::is_base_of<VBoxedValue, T>::value>::type* = nullptr>
	Any(T* pObject)
	{
		if (pObject)
		{
			mValue = Value<T>::Create(*pObject);
		}
		else
		{
			mValue = nullptr;
		}
	}

	bool IsNull() const;
	VBoxedValuePtr GetValue() const;

	Any operator[](const char* pName) const;
	Any operator[](const std::string& pName) const;

	Any& operator=(const Any& pRVal);
	Any& operator=(const nullptr_t& pRVal);
	Any& Assign(const Any& rval);

	void Swap(Any& rval);

	template<typename T>
	T As() const
	{
		return As(::GetMetaData<T>());
	}

	Any As(const VMetaData* pAsWhat) const;	
	const VMetaData* MetaData() const;

	Any operator()() const;
	Any operator()(const std::vector<Any>& pArgs) const;

	template <typename... T>
	Any operator()(const T&... pArgs) const
	{
		return Invoke({ Unpack<T>(pArgs)... });
	}
	
	template<typename T, typename std::enable_if<std::is_base_of<VBoxedValue, T>::value>::type* = nullptr>
	operator ptr<T>() const
	{
		return mValue;
	}

	template<typename T, typename std::enable_if<std::is_base_of<VObject, T>::value && !std::is_base_of<VBoxedValue, T>::value>::type* = nullptr>
	operator ptr<T>() const
	{
		const auto metaData = ::GetMetaData<T>();
		const VBoxedValuePtr result = As(metaData);

		if (result)
		{
			auto ret = reinterpret_cast<T*>(result->GetData(metaData));

			if (ret == nullptr)
			{
				LOGI("Bad cast!\n");
			}

			return ret;
		}

		return nullptr;
	}

	template<typename T, typename std::enable_if<!std::is_base_of<VObject, T>::value>::type* = nullptr>
	operator T() const
	{
		const auto metaData = ::GetMetaData<T>();
		const VBoxedValuePtr result = As(metaData);

		if (result)
		{
			return *reinterpret_cast<typename RemQual<T>::type*>(result->GetData(metaData));
		}

		throw std::runtime_error("Invalid type conversion!");
	}

	template<typename T, typename std::enable_if<!std::is_base_of<VObject, T>::value>::type* = nullptr>
	operator T*() const
	{
		const auto metaData = ::GetMetaData<T>();
		const VBoxedValuePtr result = As(metaData);

		if (result)
		{
			return reinterpret_cast<typename RemQual<T>::type*>(result->GetData(metaData));
		}

		return nullptr;
	}

	template<typename T, typename std::enable_if<std::is_base_of<VObject, T>::value>::type* = nullptr>
	operator T*() const
	{
		const auto metaData = ::GetMetaData<T>();
		const VBoxedValuePtr result = As(::GetMetaData<T>());

		if (result)
		{
			auto ret = reinterpret_cast<T*>(result->GetData(metaData));

			if (ret == nullptr)
			{
				LOGI("Bad cast!\n");
			}

			return ret;
		}

		return nullptr;
	}

	template<typename T>
	T* Cast() const
	{
		return *this;
	}	

	static Any New(const std::string& pTypeName);

	static void ReflectType(VMetaData& pMeta);
	static void RegisterTypeLua(VLuaState* pLua);
};