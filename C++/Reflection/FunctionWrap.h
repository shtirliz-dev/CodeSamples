#pragma once

class VMetaData;
class VMetaMethod;

#include <functional>
#include <Reflection/Function.h>
#include <Reflection/MetaField.h>
#include <Reflection/MetaProperty.h>
#include <Reflection/Any.h>
#include <Reflection/TypeManager.h>

template <unsigned N>
struct VMethodCall
{
	template <typename Ret, typename Class, typename... Args, typename... ArgsT>
	static Any Call(Ret(Class::*func)(Args...), RawPtr obj, Any* v, ArgsT... args)
	{
		return VMethodCall<N - 1>::Call(func, obj, v, args..., static_cast<typename std::tuple_element<sizeof...(args), std::tuple<Args...>>::type>(v[sizeof...(ArgsT)]));
	}

	template <typename Ret, typename Class, typename... Args, typename... ArgsT>
	static Any Call(Ret(Class::*func)(Args...)const, RawPtr obj, Any* v, ArgsT... args)
	{
		return VMethodCall<N - 1>::Call(func, obj, v, args..., static_cast<typename std::tuple_element<sizeof...(args), std::tuple<Args...>>::type>(v[sizeof...(ArgsT)]));
	}

	template <typename Class, typename... Args, typename... ArgsT>
	static Any Call(void(Class::*func)(Args...), RawPtr obj, Any* v, ArgsT... args)
	{
		VMethodCall<N - 1>::Call(func, obj, v, args..., static_cast<typename std::tuple_element<sizeof...(args), std::tuple<Args...>>::type>(v[sizeof...(ArgsT)]));
		return nullptr;
	}

	template <typename Class, typename... Args, typename... ArgsT>
	static Any Call(void(Class::*func)(Args...)const, RawPtr obj, Any* v, ArgsT... args)
	{
		VMethodCall<N - 1>::Call(func, obj, v, args..., static_cast<typename std::tuple_element<sizeof...(args), std::tuple<Args...>>::type>(v[sizeof...(ArgsT)]));
		return nullptr;
	}
};

template<>
struct VMethodCall<0>
{
	template <typename Ret, typename Class, typename... Args, typename... ArgsT>
	static Any Call(Ret(Class::*func)(Args...), RawPtr obj, Any* v, ArgsT... args)
	{
		return (((Class*)obj)->*func)(args...);
	}

	template <typename Ret, typename Class, typename... Args, typename... ArgsT>
	static Any Call(Ret(Class::*func)(Args...)const, RawPtr obj, Any* v, ArgsT... args)
	{
		return (((Class*)obj)->*func)(args...);
	}

	template <typename Class, typename... Args, typename... ArgsT>
	static Any Call(void(Class::*func)(Args...), RawPtr obj, Any* v, ArgsT... args)
	{
		(((Class*)obj)->*func)(args...);
		return nullptr;
	}

	template <typename Class>
	static Any Call(void(Class::*func)(), RawPtr obj)
	{
		(((Class*)obj)->*func)();
		return nullptr;
	}

	template <typename Ret, typename Class>
	static Any Call(Ret(Class::*func)(), RawPtr obj)
	{
		return (((Class*)obj)->*func)();
	}

	template <typename Ret, typename Class>
	static Any Call(Ret(Class::*func)()const, RawPtr obj)
	{
		return (((Class*)obj)->*func)();
	}

	template <typename Class>
	static Any Call(void(Class::*func)()const, RawPtr obj)
	{
		(((Class*)obj)->*func)();
		return nullptr;
	}
};

template <unsigned N>
struct VFunctionCall
{
	template <typename Ret, typename... Args, typename... ArgsT>
	static Ret Call(Ret(*func)(Args...), Any* v, ArgsT... args)
	{
		return VFunctionCall<N - 1>::Call(func, v, args..., static_cast<typename std::tuple_element<sizeof...(args), std::tuple<Args...>>::type>(v[sizeof...(ArgsT)]));
	}
};

template<>
struct VFunctionCall<0>
{
	template <typename Ret, typename... Args, typename... ArgsT>
	static Ret Call(Ret(*func)(Args...), Any* v, ArgsT... args)
	{
		return (*func)(args...);
	}
};

template <unsigned N>
struct VLambdaCall
{
    template <typename Ret, typename... Args, typename... ArgsT>
    static Ret Call(const std::function<Ret(Args...)>& func, Any* v, ArgsT... args)
    {
        return VLambdaCall<N - 1>::Call(func, v, args..., static_cast<typename std::tuple_element<sizeof...(args), std::tuple<Args...>>::type>(v[sizeof...(ArgsT)]));
    }
};

template<>
struct VLambdaCall<0>
{
    template <typename Ret, typename... Args, typename... ArgsT>
    static Ret Call(const std::function<Ret(Args...)>& func, Any* v, ArgsT... args)
    {
        return func(args...);
    }
};

template <typename Class, unsigned N>
struct VLambdaMethodCall
{
    template <typename Ret, typename... Args, typename... ArgsT>
    static Ret Call(const std::function<Ret(Class*, Args...)>& func, RawPtr obj, Any* v, ArgsT... args)
    {
        return VLambdaMethodCall<Class, N - 1>::Call(func, obj, v, args..., static_cast<typename std::tuple_element<sizeof...(args), std::tuple<Args...>>::type>(v[sizeof...(ArgsT)]));
    }

    template <typename... Args, typename... ArgsT>
    static Any Call(const std::function<void(Class*, Args...)>& func, RawPtr obj, Any* v, ArgsT... args)
    {
		VLambdaMethodCall<Class, N - 1>::Call(func, obj, v, args..., static_cast<typename std::tuple_element<sizeof...(args), std::tuple<Args...>>::type>(v[sizeof...(ArgsT)]));
        return nullptr;
    }
};

template<typename Class>
struct VLambdaMethodCall<Class, 0>
{
    template <typename Ret, typename... Args, typename... ArgsT>
    static Ret Call(const std::function<Ret(Class*, Args...)>& func, RawPtr obj, Any* v, ArgsT... args)
    {
        return func(reinterpret_cast<Class*>(obj), args...);
    }

    template <typename... Args, typename... ArgsT>
    static void Call(const std::function<void(Class*, Args...)>& func, RawPtr obj, Any* v, ArgsT... args)
    {
        return func(reinterpret_cast<Class*>(obj), args...);
    }

    template <typename Ret>
    static Ret Call(const std::function<Ret(Class*)>& func, RawPtr obj)
    {
        return func(reinterpret_cast<Class*>(obj));
    }

    static void Call(const std::function<void(Class*)>& func, RawPtr obj)
    {
        return func(reinterpret_cast<Class*>(obj));
    }
};

template <typename Sign>
class VStaticFunction;

template <typename Ret, typename... Args>
class VStaticFunction<Ret(*)(Args...)> : public VFunction
{
public:
	typedef Ret(*Signature)(Args...);

private:
	Signature mFunction = nullptr;

public:
	VStaticFunction(Signature func, const std::string& name) : VFunction(VFunctionType_Static)
	{
		mFunction = func;
		mRetType = GetMetaData<Ret>();
		mArgTypes = { GetMetaData<Args>()... };
		mName = name;
		mOwner = nullptr;
	}

	VStaticFunction(Signature func) : VFunction(VFunctionType_Static)
	{
		mFunction = func;
		mRetType = GetMetaData<Ret>();
		mArgTypes = { GetMetaData<Args>()... };
		mName = "anonymous";
		mOwner = nullptr;
	}

	Any InvokeStatic(Any* v)
	{
		return VFunctionCall<sizeof...(Args)>::Call(mFunction, v);
	}
};

template <typename... Args>
class VStaticFunction<void(*)(Args...)> : public VFunction
{
public:
	typedef void(*Signature)(Args...);

private:
	Signature mFunction = nullptr;

public:
	VStaticFunction(Signature func, const std::string& name) : VFunction(VFunctionType_Static)
	{
		mFunction = func;
		mRetType = GetMetaData<void>();
		mArgTypes = { GetMetaData<Args>()... };
		mName = name;
		mOwner = nullptr;
	}

	VStaticFunction(Signature func) : VFunction(VFunctionType_Static)
	{
		mFunction = func;
		mRetType = GetMetaData<void>();
		mArgTypes = { GetMetaData<Args>()... };
		mName = "anonymous";
		mOwner = nullptr;
	}

	Any InvokeStatic(Any* v)
	{
		VFunctionCall<sizeof...(Args)>::Call(mFunction, v);
		return nullptr;
	}
};

template <typename Sign>
class VLambdaFunction;

template <typename Ret, typename... Args>
class VLambdaFunction<Ret(*)(Args...)> : public VFunction
{
private:
    std::function<Ret(Args...)> mFunction;

public:
    VLambdaFunction(const std::function<Ret(Args...)>& func, const std::string& name) : VFunction(VFunctionType_Static)
    {
		mFunction = func;
        mRetType = GetMetaData<Ret>();
        mArgTypes = { GetMetaData<Args>()... };
        mName = name;
        mOwner = nullptr;
    }

    VLambdaFunction(const std::function<Ret(Args...)>& func) : VFunction(VFunctionType_Static)
    {
		mFunction = func;
        mRetType = GetMetaData<Ret>();
        mArgTypes = { GetMetaData<Args>()... };
        mName = "lambda";
        mOwner = nullptr;
    }

	Any InvokeStatic(Any* v)
    {
        return VLambdaCall<sizeof...(Args)>::Call(mFunction, v);
    }
};

template <typename... Args>
class VLambdaFunction<void(*)(Args...)> : public VFunction
{
private:
    std::function<void(Args...)> mFunction;

public:
    VLambdaFunction(const std::function<void(Args...)>& func, const std::string& name) : VFunction(VFunctionType_Static)
    {
		mFunction = func;
        mRetType = GetMetaData<void>();
        mArgTypes = { GetMetaData<Args>()... };
        mName = name;
        mOwner = nullptr;
    }

    VLambdaFunction(const std::function<void(Args...)>& func) : VFunction(VFunctionType_Static)
    {
		mFunction = func;
        mRetType = GetMetaData<void>();
        mArgTypes = { GetMetaData<Args>()... };
        mName = "lambda";
        mOwner = nullptr;
    }

	Any InvokeStatic(Any* v)
    {
		VLambdaCall<sizeof...(Args)>::Call(mFunction, v);
        return nullptr;
    }
};

template <typename Class, unsigned N, typename... Args>
struct VConstructorCall
{
	template <typename... ArgsT>
	static Class* Call(Any* v, ArgsT... args)
	{
		return VConstructorCall<Class, N - 1>::Call(v, args..., static_cast<typename std::tuple_element<sizeof...(args), std::tuple<Args...>>::type>(v[sizeof...(ArgsT)]));
	}
};

template<typename Class>
struct VConstructorCall<Class, 0>
{
	template <typename... ArgsT>
	static Class* Call(Any* v, ArgsT... args)
	{
		return new Class(args...);
	}

	static Class* Call(Any* v)
	{
		return new Class();
	}
};

template <typename Constructor>
class VClassConstructor;

template <typename Class, typename... Args>
class VClassConstructor<Class(Args...)> : public VFunction
{
public:
	VClassConstructor() : VFunction(VFunctionType_Constructor)
	{
		mRetType = GetMetaData<Class>();
		mArgTypes = { GetMetaData<Args>()... };
		mOwner = GetMetaData<Class>();
		mName = mOwner->GetTypeName();
	}

	Any InvokeStatic(Any* pArgs) override
	{
		return Value<Class>::CreateOwner(VConstructorCall<Class, sizeof...(Args), Args...>::Call(pArgs));
	}
};

template <typename Class>
class VClassConstructor<Class()> : public VFunction
{
public:
	VClassConstructor() : VFunction(VFunctionType_Constructor)
	{
		mRetType = GetMetaData<Class>();
		mArgTypes = {};
		mOwner = GetMetaData<Class>();
		mName = mOwner->GetTypeName();
	}

	Any InvokeStatic(Any* pArgs) override
	{
		return Value<Class>::CreateOwner(VConstructorCall<Class, 0>::Call(pArgs));
	}
};

template <typename Class>
class VClassCopyConstructor : public VFunction
{
public:
	VClassCopyConstructor() : VFunction(VFunctionType_CopyConstructor)
	{
		mRetType = GetMetaData<Class>();
		mArgTypes = { GetMetaData<Class>() };
		mOwner = GetMetaData<Class>();
		mName = mOwner->GetTypeName();
	}

	Any InvokeStatic(Any* pArgs) override
	{
		Class* second = pArgs[0];
		return new Class(*second);
	}
};

template <typename Class>
class VClassOperatorAssigment : public VFunction
{
public:
	VClassOperatorAssigment() : VFunction(VFunctionType_OperatorAssigment)
	{
		mRetType = GetMetaData<void>();
		mArgTypes = { GetMetaData<Class>(), GetMetaData<Class>() };
		mOwner = GetMetaData<Class>();
		mName = mOwner->GetTypeName();
	}

	Any InvokeStatic(Any* pArgs) override
	{
		Class* left = pArgs[0];
		Class* right = pArgs[1];
		*left = *right;
		return nullptr;
	}
};

template <typename Sign>
class VClassMethod;

template <typename Sign>
class VClassField;

template <typename Member>
class VBoxedClassField;

template <typename Member>
class VBoxedClassField<ptr<Member>> : public VBoxedValue
{
private:
	Any mParent;
	ptr<Member>* mField;

public:
	VBoxedClassField(const Any& pParent, ptr<Member>* pField) :
		mParent(pParent),
		mField(pField),
		VBoxedValue(::GetMetaData<Member>())
	{
	}

	RawPtr GetData(const VMetaData* pTargetType) const override
	{
		return mMetaData->CastPointer(mField->Get(), pTargetType);
	}

	bool IsUnboxable() const override
	{
		return true;
	}

	VBoxedValuePtr Get() override
	{
		return Value<Member>::Create(mField->Get());
	}

	void Set(const VBoxedValuePtr& pValue) override
	{
		if (mMetaData == pValue->GetMetaData())
		{
			*mField = static_cast<Member*>(pValue->GetData(mMetaData));
		}
		else
		{
			const auto converted = VTypeManager::Instance().Convert(pValue, mMetaData);

			if (converted)
			{
				*mField = static_cast<Member*>(converted->GetData(mMetaData));
			}
			else
			{
				throw std::runtime_error("Invalid type conversion!");
			}
		}
	}
};

template <typename Member>
class VBoxedClassField : public VBoxedValue
{
private:
	Any mParent;
	Member* mField;

public:
	VBoxedClassField(const Any& pParent, Member* pField) :
		mParent(pParent),
		mField(pField),
		VBoxedValue(::GetMetaData<Member>())
	{
	}

	RawPtr GetData(const VMetaData* pTargetType) const override
	{
		return mMetaData->CastPointer(mField, pTargetType);
	}

	bool IsUnboxable() const override
	{
		return false;
	}

	VBoxedValuePtr Get() override
	{
		return Value<Member>::Create(*mField);
	}

	void Set(const VBoxedValuePtr& pValue) override
	{
		if (mMetaData == pValue->GetMetaData())
		{
			*mField = *static_cast<Member*>(pValue->GetData(mMetaData));
		}
		else
		{
			const auto converted = VTypeManager::Instance().Convert(pValue, mMetaData);
			
			if (converted)
			{
				*mField = *static_cast<Member*>(converted->GetData(mMetaData));
			}
			else
			{
				throw std::runtime_error("Invalid type conversion!");
			}
		}
	}
};

template <typename Class, typename Member>
class VClassField<Member Class::*> : public VClassMember
{
public:
	typedef Member Class::*Signature;

private:
	Signature mMember = nullptr;

public:
	VClassField(Member Class::*pMember, const std::string& pName,
		const std::unordered_map<std::string, Any>& pAttributes)
	{
		mMember = pMember;
		mMetaData = GetMetaData<Member>();
		mName = pName;
		mType = VClassMemberType_Field;
		mAttributes = pAttributes;
	}
		
	Any Get(const Any& pObject) const override
	{
		const auto data = pObject.GetValue()->GetData(GetMetaData<Class>());
		const auto clazz = reinterpret_cast<Class*>(data);
		const auto memberPtr = &(clazz->*mMember);
		return new VBoxedClassField<Member>(pObject, memberPtr);
	}
};

template <typename Class, typename Ret, typename... Args>
class VClassMethod<Ret(Class::*)(Args...)> : public VFunction
{
public:
	typedef Ret(Class::*Signature)(Args...);

private:
	Signature mFunction = nullptr;

public:
	VClassMethod(Signature func, const std::string& name = "") : VFunction(VFunctionType_ClassMethod)
	{
		mFunction = func;
		mArgTypes = { GetMetaData<Args>()... };
		mRetType = GetMetaData<Ret>();
		mOwner = GetMetaData<Class>();
		mName = name;
	}

	Any InvokeMethod(RawPtr obj, Any* v)
	{
		return VMethodCall<sizeof...(Args)>::Call(mFunction, obj, v);
	}
};

template <typename Class, typename Ret, typename... Args>
class VClassMethod<Ret(Class::*)(Args...)const> : public VFunction
{
public:
	typedef Ret(Class::*Signature)(Args...)const;

private:
	Signature mFunction = nullptr;

public:
	VClassMethod(Signature func, const std::string& name = "") : VFunction(VFunctionType_ClassMethod)
	{
		mFunction = func;
		mArgTypes = { GetMetaData<Args>()... };
		mRetType = GetMetaData<Ret>();
		mOwner = GetMetaData<Class>();
		mName = name;
	}

	Any InvokeMethod(RawPtr obj, Any* v)
	{
		return VMethodCall<sizeof...(Args)>::Call(mFunction, obj, v);
	}
};

template <typename Class, typename Ret>
class VClassMethod<Ret(Class::*)()> : public VFunction
{
public:
	typedef Ret(Class::*Signature)();

private:
	Signature mFunction = nullptr;

public:
	VClassMethod(Signature func, const std::string& name = "") : VFunction(VFunctionType_ClassMethod)
	{
		mFunction = func;
		mRetType = GetMetaData<Ret>();
		mOwner = GetMetaData<Class>();
		mName = name;
	}

	Any InvokeMethod(RawPtr obj, Any* v)
	{
		return VMethodCall<0>::Call(mFunction, obj);
	}
};

template <typename Class, typename Ret>
class VClassMethod<Ret(Class::*)()const> : public VFunction
{
public:
	typedef Ret(Class::*Signature)()const;

private:
	Signature mFunction = nullptr;

public:
	VClassMethod(Signature func, const std::string& name = "") : VFunction(VFunctionType_ClassMethod)
	{
		mFunction = func;
		mRetType = GetMetaData<Ret>();
		mOwner = GetMetaData<Class>();
		mName = name;
	}

	Any InvokeMethod(RawPtr obj, Any* v)
	{
		return VMethodCall<0>::Call(mFunction, obj);
	}
};

template <typename Signature>
class VClassMethodExternal;

template <typename Ret, typename Type, typename... Args>
class VClassMethodExternal<Ret(Type::*)(Args...) const> : public VFunction
{
private:
	std::function<Ret(Args...)> mFunction;
	typedef typename RemQual<typename std::tuple_element<0, std::tuple<Args...>>::type>::type ClassType;

public:
	VClassMethodExternal(const std::function<Ret(Args...)>& func, const std::string& name) : VFunction(VFunctionType_ClassMethodExternal)
	{
		mFunction = func;
		mRetType = GetMetaData<Ret>();
		mArgTypes = { GetMetaData<Args>()... };
		mName = name;
		mOwner = GetMetaData<ClassType>();
	}

	Any InvokeMethod(RawPtr obj, Any* args)
	{
		auto thiz = reinterpret_cast<ClassType*>(obj);
		return VLambdaMethodCall<ClassType, sizeof...(Args)-1>::Call(mFunction, obj, args);
	}
};

template <typename Type, typename... Args>
class VClassMethodExternal<void(Type::*)(Args...) const> : public VFunction
{
private:
	std::function<void(Args...)> mFunction;
	typedef typename RemQual<typename std::tuple_element<0, std::tuple<Args...>>::type>::type ClassType;

public:
	VClassMethodExternal(const std::function<void(Args...)>& func, const std::string& name) : VFunction(VFunctionType_ClassMethodExternal)
	{
		mFunction = func;
		mRetType = GetMetaData<void>();
		mArgTypes = { GetMetaData<Args>()... };
		mName = name;
		mOwner = GetMetaData<ClassType>();
	}

	Any InvokeMethod(RawPtr obj, Any* args)
	{
		auto thiz = reinterpret_cast<ClassType*>(obj);
		VLambdaMethodCall<ClassType, sizeof...(Args)-1>::Call(mFunction, obj, args);
		return nullptr;
	}
};