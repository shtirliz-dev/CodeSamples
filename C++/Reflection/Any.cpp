#include "Reflection/TypeManager.h"
#include "Containers/Pool.h"
#include "Reflection/FunctionWrap.h"

Any::Any()
{
}

Any::Any(const char* pString)
{
	mValue = Value<std::string>::Create(pString);
}

Any::Any(const wchar_t* pString)
{
	mValue = Value<std::wstring>::Create(pString);
}

Any::Any(const nullptr_t& pNullPointer)
{
	mValue = pNullPointer;
}

Any::Any(const Any& pAny)
{
	mValue = pAny.Unbox();
}

Any::Any(const VBoxedValuePtr& pBox)
{
	if (pBox)
	{
		if (pBox->IsUnboxable())
		{
			mValue = pBox->Get();
		}
		else
		{
			mValue = pBox;
		}
	}
}

bool Any::IsNull() const
{
	return !Unbox();
}

VBoxedValuePtr Any::GetValue() const
{
	return Unbox();
}

const VMetaData* Any::MetaData() const
{
	const auto value = Unbox();

	if (value)
	{
		return value->GetMetaData();
	}

	return nullptr;
}

Any Any::Invoke() const
{
	return Unbox()->Invoke();
}

Any Any::Invoke(const std::vector<Any>& pArgs) const
{
	return Unbox()->Invoke(pArgs);
}

Any Any::operator()() const
{
	return Invoke();
}

Any Any::operator()(const std::vector<Any>& pArgs) const
{
	return Invoke(pArgs);
}

Any Any::GetMember(const std::string& pName) const
{
	const auto value = Unbox();

	if (const auto metaData = value->GetMetaData())
	{
		if (const auto member = metaData->GetMember(pName))
		{
			return member->Get(value);
		}
	}

	return nullptr;
}

Any Any::operator[](const std::string& pName) const
{
	return GetMember(pName);
}

Any Any::operator[](const char* pName) const
{
	return GetMember(pName);
}

VBoxedValuePtr Any::Unbox() const
{
	if (mValue)
	{
		if (mValue->IsUnboxable())
		{
			return mValue->Get();
		}
		else
		{
			return mValue;
		}
	}

	return nullptr;
}

Any Any::As(const VMetaData* pAsWhat) const
{
	if (pAsWhat == nullptr)
	{
		return nullptr;
	}
	else if (pAsWhat == GetMetaData<Any>())
	{
		return Unbox();
	}
	else if (pAsWhat == GetMetaData<RawPtr>())
	{
		return Unbox();
	}
	else
	{
		const auto metaData = MetaData();

		if (metaData == nullptr)
		{
			return nullptr;
		}
		else if (metaData == pAsWhat)
		{
			return Unbox();
		}
		else
		{
			auto& typeManager = VTypeManager::Instance();

			if (typeManager.CanConvert(metaData, pAsWhat))
			{
				return typeManager.Convert(Unbox(), pAsWhat);
			}
		}
	}

	return nullptr;
}

Any& Any::operator=(const nullptr_t& rval)
{
	mValue = nullptr;
	return *this;
}

Any Any::New(const std::string& pTypeName)
{
	const auto metaData = VTypeManager::Instance().GetMetaData(pTypeName);

	if (metaData != nullptr)
	{
		return metaData->New();
	}
	
	return nullptr;
}

Any& Any::operator=(const Any& rval)
{
	if (mValue)
	{
		mValue->Set(rval.Unbox());
	}
	else
	{
		mValue = rval.Unbox();
	}
			
	return *this;
}

Any& Any::Assign(const Any& rval)
{
	if (mValue)
	{
		const auto thisMeta = MetaData();
		mValue->Set(rval.As(thisMeta));
	}
	else
	{
		mValue = rval.Unbox();
	}

	return *this;
}

void Any::Swap(Any& rval)
{
	std::swap(mValue, rval.mValue);
}

VBoxedValue::VBoxedValue(const VMetaData* pMetaData) : mMetaData(pMetaData)
{
}

RawPtr VBoxedValue::GetData(const VMetaData* pTargetType) const
{
	return nullptr;
}

VBoxedValuePtr VBoxedValue::Invoke()
{
	return nullptr;
}

VBoxedValuePtr VBoxedValue::Invoke(const std::vector<Any>& pArgs)
{
	return nullptr;
}

VBoxedValuePtr VBoxedValue::Get()
{
	return ptr<VBoxedValue>(this);
}

void VBoxedValue::Set(const VBoxedValuePtr& pValue)
{
	V_ASSERT(false);
}

const VMetaData* VBoxedValue::GetMetaData() const
{
	return mMetaData;
}

bool VBoxedValue::IsDataOwner() const
{
	return false;
}

bool VBoxedValue::IsUnboxable() const
{
	return false;
}

VPool gBoxedValuePool(16384, 64);

void* VBoxedValue::operator new(size_t size)
{
	V_ASSERT(size <= 64);

	auto result = gBoxedValuePool.Alloc();
	
	if (result == nullptr)
	{
		LOGE("Pool overflow, good bye!");
		exit(0);
	}

	return result;
}

void VBoxedValue::operator delete(void* data)
{
	gBoxedValuePool.Dealloc(data);
}

VBoxedHolder::VBoxedHolder(RawPtr pData, const VMetaData* pMetaData) :
	VBoxedValue(pMetaData),
	mData(pData)
{
}

void VBoxedHolder::Set(const VBoxedValuePtr& pData)
{
	mMetaData->Move(mData, pData->GetData(mMetaData));
}

RawPtr VBoxedHolder::GetData(const VMetaData* pTargetType) const
{
	return reinterpret_cast<RawPtr>(mMetaData->CastPointer(mData, pTargetType));
}

void Any::ReflectType(VMetaData& pMeta)
{
	pMeta.AddStaticFunction("RegisterTypeLua", &Any::RegisterTypeLua);
}

void Any::RegisterTypeLua(VLuaState* pLua)
{
	auto unboxAny = [](Any& any, const std::string& type) -> sol::object
	{
		if (auto metaData = VTypeManager::Instance().GetMetaData(type))
		{
			auto value = any.As(metaData).GetValue();

			if (value)
			{
				lua_State* state = VLua::Instance()->GetState();
				RawPtr data = value->GetData(metaData);
				sol::object result = metaData->GetTypeTable()->GetLuaObject(state, data);
				return result;
			}
		}
		else
		{
			LOGI("Any::Get: unregistered type: '%s'!", type.c_str());
		}

		return sol::nil;
	};

	auto boxAny = [](Any& any, const std::string& type, const sol::object& value)
	{
		if (auto metaData = VTypeManager::Instance().GetMetaData(type))
		{
			auto typeTable = metaData->GetTypeTable();

			if (typeTable->IsLuaType(value))
			{
				any.Assign(Any(typeTable->GetBoxedValue(value)));
			}
		}
		else
		{
			LOGI("Any::Set: unregistered type: '%s'!", type.c_str());
		}
	};

	auto invoke = [](Any& method, sol::variadic_args args) -> Any
	{
		auto boxedValue = method.GetValue().DynamicCast<VBoxedClassMethod>();

		if (boxedValue)
		{
			auto unboxedMethod = boxedValue.Get();

			if (unboxedMethod)
			{
				auto func = unboxedMethod->GetMethod()->Method();
				auto argsCount = func->GetArgumentsCount();

				if (argsCount == 0)
				{
					return method();
				}
				else
				{
					std::vector<Any> anyArgs;
					anyArgs.resize(argsCount);

					for (size_t i = 0; i < argsCount && i < args.size(); i++)
					{
						auto argType = func->GetArgumentType(i);
						auto typeTable = argType->GetTypeTable();

						sol::object luaArg = args[i];

						if (luaArg.is<Any>())
						{
							anyArgs[i] = luaArg.as<Any>();
						}
						else if (typeTable->IsLuaType(luaArg))
						{
							anyArgs[i].Assign(typeTable->GetBoxedValue(luaArg));
						}
						else
						{
							V_ASSERT(false);
						}
					}

					return method(anyArgs);
				}
			}
		}

		return nullptr;
	};

	auto setProperty = [](Any& any, const char* name, sol::object value)
	{
		if (any.IsNull())
		{
			LOGI("Invalid property assigment from Lua: target is null!");
		}

		auto boxedValue = any[name];

		if (!boxedValue.IsNull())
		{
			auto unboxedMethod = boxedValue.MetaData();

			if (unboxedMethod == nullptr)
			{
				LOGI("Invalid property assigment from Lua: meta data is null!");
				V_ASSERT(false);
				return;
			}

			auto typeTable = unboxedMethod->GetTypeTable();

			if (value.is<Any>())
			{
				boxedValue.Assign(value.as<Any>());
			}
			else if (typeTable->IsLuaType(value))
			{
				boxedValue.Assign(typeTable->GetBoxedValue(value));
			}
			else
			{
				LOGI("Invalid property assigment from Lua: %s::%s(%s) argument type is invalid!", 
					unboxedMethod->GetTypeName().c_str(), 
					name, 
					unboxedMethod->GetTypeName().c_str());

				V_ASSERT(false);
			}
		}
		else
		{
			LOGI("Invalid property assigment from Lua: no such property: '%s'!", name);
		}
	};

	pLua->new_usertype<Any>("Any",
		sol::constructors<Any(), Any(int), Any(float), Any(bool), Any(const std::string&), Any(const Any&)>(),
		"Get", unboxAny,
		"Set", boxAny,
		"Invoke", invoke,
		"IsNull", &Any::IsNull,
		"New", &Any::New,
		sol::meta_function::index, static_cast<Any(Any::*)(const char*)const>(&Any::operator[]),
		sol::meta_function::call, invoke,
		sol::meta_function::new_index, setProperty);
}

REGISTER_TYPE(Any);