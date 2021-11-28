#pragma once

#include <Scripting/Lua.h>
#include <Reflection/Object.h>
#include <Reflection/MetaMethod.h>
#include <Reflection/MetaProperty.h>

class Any;
class VMetaData;
class VMetaMethod;
class VClassMember;
class VMetaField;
class VMetaProperty;
class VFunction;
class VMetaMethod;
class VBoxedValue;

template <typename Sign>
class VClassMethod;

template <typename Sign>
class VClassField;

template <typename Sign>
class VStaticFunction;

template <typename Signature>
class VClassMethodExternal;

template <typename Constructor>
class VClassConstructor;

template <typename T>
std::pair<std::string, Any> UnpackAttribute(const T& pAttr);

template<typename T, typename U>
class Value;

typedef void(*VMetaDataConstructor)();

struct VTypeTable
{
	int(*GetSize)() = nullptr;
	void(*StaticNew)(void**) = nullptr;
	void(*Construct)(void**) = nullptr;
	void(*StaticDelete)(void**) = nullptr;
	void(*Destruct)(void**) = nullptr;
	void(*Clone)(void* const*, void**) = nullptr;
	void(*Move)(void* const*, void**) = nullptr;
	VLuaObject(*GetLuaObject)(lua_State*, void*) = nullptr;
	ptr<VBoxedValue>(*GetBoxedValue)(const VLuaObject& obj) = nullptr;
	bool(*IsLuaType)(const VLuaObject&) = nullptr;
	void(*ReflectType)(VMetaData&) = nullptr;
};

class VInheritanceHelperBase
{
private:
	VMetaData* mBaseMetaData;
	VMetaData* mDerivedMetaData;

public:
	VInheritanceHelperBase(VMetaData* pBaseMetaData, VMetaData* pDerivedMetaData)
	{
		mBaseMetaData = pBaseMetaData;
		mDerivedMetaData = pDerivedMetaData;
	}

	VMetaData* BaseMetaData() { return mBaseMetaData; }
	VMetaData* DerivedMetaData() { return mDerivedMetaData; }

	virtual void* BaseToDerived(void* pPtr) = 0;
	virtual void* DerivedToBase(void* pPtr) = 0;
};

template<typename T, typename U = void>
class VTypeFuncs;

template<typename T>
struct VTypeFuncs<T, typename std::enable_if<std::is_base_of<VObject, T>::value>::type>
{
	static int GetSize()
	{
		return sizeof(T);
	}

	static void StaticNew(void** dest)
	{
		*dest = new T();
	}

	static void Construct(void** dest)
	{
		new (*dest) T();
	}

	static void StaticDelete(void** x)
	{
		delete (*reinterpret_cast<T**>(x));
	}

	static void Destruct(void** x)
	{
		(*reinterpret_cast<T**>(x))->~T();
	}

	static void Clone(void* const* src, void** dest)
	{
		*dest = new T(**reinterpret_cast<T* const*>(src));
	}

	static void Move(void* const* src, void** dest)
	{
		**reinterpret_cast<T**>(dest) = **reinterpret_cast<T* const*>(src);
	}

	static VLuaObject GetLuaObject(lua_State* state, void* from)
	{
		return sol::make_object(state, reinterpret_cast<T*>(from));
	}

	static ptr<VBoxedValue> GetBoxedValue(const VLuaObject& obj)
	{
		return Value<T>::Create(obj.as<T*>());
	}

	static bool IsLuaType(const VLuaObject& obj)
	{
		return obj.is<T>();
	}
};

template<typename T>
struct VTypeFuncs<T, typename std::enable_if<!std::is_base_of<VObject, T>::value>::type>
{
	static int GetSize()
	{
		return sizeof(T);
	}

	static void StaticNew(void** dest)
	{
		*dest = new T();
	}

	static void Construct(void** dest)
	{
		new (*dest) T();
	}

	static void StaticDelete(void** x)
	{
		delete (*reinterpret_cast<T**>(x));
	}

	static void Destruct(void** x)
	{
		(*reinterpret_cast<T**>(x))->~T();
	}

	static void Clone(void* const* src, void** dest)
	{
		*dest = new T(**reinterpret_cast<T* const*>(src));
	}

	static void Move(void* const* src, void** dest)
	{
		**reinterpret_cast<T**>(dest) = **reinterpret_cast<T* const*>(src);
	}

	static VLuaObject GetLuaObject(lua_State* state, void* from)
	{
		return sol::make_object(state, *reinterpret_cast<T*>(from));
	}

	static ptr<VBoxedValue> GetBoxedValue(const VLuaObject& obj)
	{
		return Value<T>::Create(obj.as<T>());
	}

	static bool IsLuaType(const VLuaObject& obj)
	{
		return obj.is<T>();
	}
};

#define REGISTER_TYPE(Name) VTypeInfo<Name> gMeta ## Name(#Name, &Name::ReflectType);
#define REGISTER_TYPE_RENAMED(Name, CustomName) VTypeInfo<Name> gMeta ## CustomName(#CustomName, &Name::ReflectType);
#define REGISTER_TYPE_CUSTOM(Name, FunctionName) VTypeInfo<Name> gMeta ## Name(#Name, &FunctionName);
#define REGISTER_TYPE_CUSTOM_RENAMED(Name, CustomName, FunctionName) VTypeInfo<Name> gMeta ## CustomName(#CustomName, &FunctionName);

class VFuncArgumentMatchFinder
{
private:
	VMetaMethod* mResult = nullptr;
	std::vector<VMetaMethod*> mAcceptable;

public:
	VFuncArgumentMatchFinder() = delete;
	VFuncArgumentMatchFinder(const std::vector<VMetaMethod*>& pFunctions, const std::string& pName, const std::vector<VMetaData*>& pGivenArguments);
	VFuncArgumentMatchFinder(const std::vector<VMetaMethod*>& pFunctions, const std::string& pName, const std::vector<Any>& pGivenArguments);
	VMetaMethod* Result();
};

template<typename T>
VMetaData* GetMetaData();

template<class Base, class Derived>
class VInheritanceHelper : public VInheritanceHelperBase
{
public:
	VInheritanceHelper() : VInheritanceHelperBase(GetMetaData<Base>(), GetMetaData<Derived>()) { }

	RawPtr BaseToDerived(RawPtr pPtr) final
	{
		return static_cast<RawPtr>(dynamic_cast<Derived*>(static_cast<Base*>(pPtr)));
	}

	RawPtr DerivedToBase(RawPtr pPtr) final
	{
		return static_cast<RawPtr>(dynamic_cast<Base*>(static_cast<Derived*>(pPtr)));
	}
};

class VMetaData : public VObject
{
	friend class VTypeManager;

private:
	static size_t mTypeIdCounter;

	size_t mTypeId = 0;
	std::string mTypeName;
	std::string mTypeIdName;
	size_t mSizeOf = 0;
	
	std::vector<VClassMember*> mMergedMembers;
	std::vector<VClassMember*> mMembers;

	std::vector<VMetaMethod*> mMethods;
	std::vector<VClassMember*> mFields;
	std::vector<VMetaProperty*> mProperties;

	std::unordered_map<std::string, Any> mAttributes;
	std::vector<VInheritanceHelperBase*> mBaseClasses;
	std::unique_ptr<VTypeTable> mTypeTable;

	mutable VFunction* mMetaDataGetter = nullptr;
	mutable VFunction* mDefaultConstructor = nullptr;
	mutable VFunction* mCopyConstructor = nullptr;
	mutable VFunction* mAssigmentOperator = nullptr;

public:
	VMetaData();
	~VMetaData();

	void Init(const std::string& pTypeName, size_t pSizeOf);
	
	VClassMember* GetMember(size_t index) const;
	VMetaField* GetField(size_t index) const;
	VMetaProperty* GetProperty(size_t index) const;
	VClassMember* GetMember(const std::string& pName) const;

	const std::vector<VClassMember*>& GetMembers() const;
	const std::vector<VMetaMethod*>& GetMethods() const;
	const std::vector<VClassMember*>& GetFields() const;
	const std::vector<VMetaProperty*>& GetProperties() const;

	std::vector<VMetaProperty*> GetPropertiesByTag(const std::string& pTag) const;
	std::vector<VMetaMethod*> GetMethodsByTag(const std::string& pTag) const;

	size_t GetSizeOf() const;
	size_t GetFieldsCount() const;
	size_t GetPropertiesCount() const;	
	size_t GetMethodsCount() const;
	size_t GetMembersCount() const;

	size_t GetTypeId() const;
	const std::string& GetTypeName() const;
	const std::string& GetTypeIdName() const;
	
	VInheritanceHelperBase* IsDerivedFrom(const VMetaData* pMetaData) const;
		
	bool InClassHierarchy() const;

	RawPtr CastPointer(RawPtr pPointer, const VMetaData* pTargetType) const;

	VTypeTable* GetTypeTable() const;
	VMetaData* GetMetaData(void* object);
	VLuaObject GetLuaObject(lua_State* state, RawPtr object) const;

	ptr<VBoxedValue> New() const;
	RawPtr Copy(RawPtr pOther) const;
	void Move(RawPtr pWhere, RawPtr pWhat) const;
	void Delete(RawPtr pOther) const;
		
	void SetMetaDataGetter(VFunction* pMethod);
	void ShowDebug();

	bool IsInitialized() const;
	bool IsManagedType() const;
	bool HasMethod(const std::string& name) const;
	bool HasAttribute(const std::string& pAttribute) const;
	const Any& GetAttribute(const std::string& pAttribute) const;

	VMetaData& AddElement(VClassMember* pElement);
	VMetaData& AddElement(VInheritanceHelperBase* pBaseClass);

	VMetaData& SetName(const std::string& pName);

	template<typename... Attributes>
	VMetaData& SetAttributes(const Attributes&... pAttributes)
	{
		mAttributes =
		{
			UnpackAttribute<Attributes>(pAttributes)...
		};

		return*this;
	}

	template<typename Constructor>
	VMetaData& AddConstructor()
	{
		return AddElement(new VMetaMethod(new VClassConstructor<Constructor>(), std::unordered_map<std::string, Any>{}));
	}

	template<typename Base, typename Derived>
	VMetaData& AddBaseClass()
	{
		return AddElement(new VInheritanceHelper<Base, Derived>());
	}

	template<typename Signature, typename... Attributes>
	VMetaData& AddMethod(const std::string& pName, Signature pMethod, const Attributes&... pAttributes)
	{
		THROW_ASSERT(!pName.empty() && pMethod);
		return AddElement(new VMetaMethod(new VClassMethod<Signature>(pMethod, pName), 
			std::unordered_map<std::string, Any>
			{ 
				UnpackAttribute<Attributes>(pAttributes)...
			}));
	}

	template<typename Signature, typename... Attributes>
	VMetaData& AddMethodExternal(const std::string& pName, const Signature& pMethod, const Attributes&... pAttributes)
	{
		THROW_ASSERT(!pName.empty());
		return AddElement(new VMetaMethod(new VClassMethodExternal<decltype(&Signature::operator())>(pMethod, pName), 
			std::unordered_map<std::string, Any>
			{ 
				UnpackAttribute<Attributes>(pAttributes)...
			}));
	}

	template<typename GetSignature, typename SetSignature, typename... Attributes>
	VMetaData& AddProperty(const std::string& pName, GetSignature pGetter, SetSignature pSetter, const Attributes&... pAttributes)
	{
		THROW_ASSERT(!pName.empty() && (pGetter || pSetter));
		return AddElement(new VMetaProperty(pName, 
			pGetter ? new VClassMethod<GetSignature>(pGetter) : nullptr,
			pSetter ? new VClassMethod<SetSignature>(pSetter) : nullptr, 
			std::unordered_map<std::string, Any>
			{ 
				UnpackAttribute<Attributes>(pAttributes)...
			}));
	}

	template<typename Class, typename Member, typename... Attributes>
	VMetaData& AddField(const std::string& pName, Member Class::*pMember, const Attributes&... pAttributes)
	{
		THROW_ASSERT(!pName.empty() && pMember);
		return AddElement(new VClassField<decltype(pMember)>(pMember, pName, 
			std::unordered_map<std::string, Any>
			{ 
				UnpackAttribute<Attributes>(pAttributes)...
			}));
	}

	template<typename Signature, typename... Attributes>
	VMetaData& AddStaticFunction(const std::string& pName, Signature pMethod, const Attributes&... pAttributes)
	{
		THROW_ASSERT(!pName.empty() && pMethod);
		return AddElement(new VMetaMethod(new VStaticFunction<Signature>(pMethod, pName), 
			std::unordered_map<std::string, Any>
			{ 
				UnpackAttribute<Attributes>(pAttributes)...
			}));
	}

	template<typename Signature, typename... Attributes>
	VMetaData& AddConversion(Signature pMethod, const Attributes&... pAttributes)
	{
		return AddStaticFunction("(cast)", pMethod, std::forward<Attributes>(pAttributes)...);
	}

	template<typename Signature, typename... Attributes>
	VMetaData& AddUnaryOperator(const std::string& pName, Signature pMethod, const Attributes&... pAttributes)
	{
		return AddStaticFunction(pName, pMethod, std::forward<Attributes>(pAttributes)...);
	}

	template<typename Signature, typename... Attributes>
	VMetaData& AddBinaryOperator(const std::string& pName, Signature pMethod, const Attributes&... pAttributes)
	{
		return AddStaticFunction(pName, pMethod, std::forward<Attributes>(pAttributes)...);
	}
	
private:
	VMetaData(const VMetaData& other) = delete;
	VMetaData(VMetaData&& other) = delete;
};

class VTypeRegistrationHelper
{
public:
	static void RegisterType(VMetaData* pMetaData);
};

template <typename X>
class VTypeInfo
{
public:
	VTypeInfo(const std::string& name, void(*pRegisterCallback)(VMetaData&) = nullptr)
	{
		auto metaData = MetaData();

		if (!metaData->IsInitialized())
		{
			V_ASSERT(false);
			return;
		}

		auto table = metaData->GetTypeTable();

		table->Destruct = VTypeFuncs<X>::Destruct;
		table->StaticDelete = VTypeFuncs<X>::StaticDelete;
		table->GetLuaObject = VTypeFuncs<X>::GetLuaObject;
		table->GetBoxedValue = VTypeFuncs<X>::GetBoxedValue;
		table->IsLuaType = VTypeFuncs<X>::IsLuaType;
		table->ReflectType = pRegisterCallback;

		metaData->SetName(name);

		VTypeRegistrationHelper::RegisterType(metaData);
	}

	static VMetaData* MetaData()
	{
		static ptr<VMetaData> sMetaData;

		if (sMetaData == nullptr)
		{
			sMetaData = new VMetaData();
			sMetaData->Init(typeid(X).name(), sizeof(X));
		}

		return sMetaData;
	}
};

template <>
class VTypeInfo<void>
{
public:
	static VMetaData* MetaData()
	{
		static ptr<VMetaData> sMetaData;

		if (sMetaData == nullptr)
		{
			sMetaData = new VMetaData();
			sMetaData->Init("void", 0);
		}

		return sMetaData;
	}
};

template<typename T>
VMetaData* GetMetaData()
{
	return VTypeInfo<typename RemQual<T>::type>::MetaData();
}