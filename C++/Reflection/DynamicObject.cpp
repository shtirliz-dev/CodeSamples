
#include "DynamicObject.h"
#include "Reflection/FunctionWrap.h"
#include "Reflection/Attribute.h"

void VDynamicObject::SetField(const std::string& pName, const Any& pValue)
{
	if (pValue.IsNull())
	{
		mFields.erase(pName);
	}
	else
	{
		mFields[pName].Assign(pValue);
	}
}

Any VDynamicObject::GetField(const std::string& pName) const
{
	const auto result = mFields.find(pName);

	if (result != mFields.end())
	{
		return result->second;
	}

	return nullptr;
}

bool VDynamicObject::HasField(const std::string& pName) const
{
	const auto result = mFields.find(pName);

	if (result != mFields.end())
	{
		return true;
	}

	return false;
}

std::vector<std::string> VDynamicObject::GetFieldNames() const
{
	std::vector<std::string> result;

	result.reserve(mFields.size());

	for (const auto& field : mFields)
	{
		result.push_back(field.first);
	}

	return result;
}

size_t VDynamicObject::GetFieldsCount() const
{
	return mFields.size();
}

bool VDynamicObject::Empty() const
{
	return mFields.empty();
}

VDynamicObjectProtocol::VDynamicObjectProtocol() : DynamicObject(new VDynamicObject())
{
}

VDynamicObjectProtocol::~VDynamicObjectProtocol()
{
}

void VDynamicObjectProtocol::ReflectType(VMetaData& pMeta)
{
	pMeta.AddConstructor<VDynamicObjectProtocol()>();
	pMeta.AddField("DynamicObject", &VDynamicObjectProtocol::DynamicObject, VAttribute("Hidden"));
}

REGISTER_TYPE(VDynamicObjectProtocol);

void VDynamicObject::ReflectType(VMetaData& pMeta)
{
	pMeta.AddBaseClass<VObject, VDynamicObject>();
	pMeta.AddConstructor<VDynamicObject()>();
	pMeta.AddMethod("SetField", &VDynamicObject::SetField);
	pMeta.AddMethod("GetField", &VDynamicObject::GetField);
	pMeta.AddMethod("HasField", &VDynamicObject::HasField);
	pMeta.AddMethod("GetFieldsCount", &VDynamicObject::GetFieldsCount);
	pMeta.AddMethod("Empty", &VDynamicObject::Empty);
	pMeta.AddMethod("GetFieldNames", &VDynamicObject::GetFieldNames);
	pMeta.AddField("mFields", &VDynamicObject::mFields, VAttribute("Hidden"));
}

REGISTER_TYPE(VDynamicObject);

void RegisterStringAnyMap(VMetaData& pMeta)
{
	pMeta.AddConstructor<StringAnyMap()>();
	pMeta.AddConstructor<StringAnyMap(const StringAnyMap&)>();

	pMeta.AddMethodExternal("Range", [](StringAnyMap* vec) -> std::vector<Any>
	{
		std::vector<Any> keys;
		keys.reserve(vec->size());
		for (auto& elem : *vec)
		{
			keys.push_back(elem.first);
		}
		return keys;
	});

	pMeta.AddMethodExternal("At", [](StringAnyMap* vec, const Any& index) -> Any&
	{
		return (*vec)[index];
	});

	pMeta.AddMethodExternal("Set", [](StringAnyMap* vec, const Any& index, const Any& value) -> void
	{
		(*vec)[index] = value;
	});

	pMeta.AddMethodExternal("Size", [](StringAnyMap* vec) -> int
	{
		return (int)vec->size();
	});
}

VTypeInfo<StringAnyMap> gMetaStringAnyMap("StringAnyMap", &RegisterStringAnyMap);

void RegisterStringAnyUMap(VMetaData& pMeta)
{
	pMeta.AddConstructor<StringAnyUMap()>();
	pMeta.AddConstructor<StringAnyUMap(const StringAnyUMap&)>();

	pMeta.AddMethodExternal("Range", [](StringAnyUMap* vec) -> std::vector<Any>
	{
		std::vector<Any> keys;
		keys.reserve(vec->size());
		for (auto& elem : *vec)
		{
			keys.push_back(elem.first);
		}
		return keys;
	});

	pMeta.AddMethodExternal("At", [](StringAnyUMap* vec, const Any& index) -> Any&
	{
		return (*vec)[index];
	});

	pMeta.AddMethodExternal("Set", [](StringAnyUMap* vec, const Any& index, const Any& value) -> void
	{
		(*vec)[index] = value;
	});

	pMeta.AddMethodExternal("Size", [](StringAnyUMap* vec) -> int
	{
		return (int)vec->size();
	});
}

VTypeInfo<StringAnyUMap> gMetaStringAnyUMap("StringAnyUMap", &RegisterStringAnyUMap);