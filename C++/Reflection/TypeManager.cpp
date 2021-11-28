
#include "Reflection/TypeManager.h"
#include "Reflection/Function.h"
#include "Containers/Array.h"
#include "Reflection/FunctionWrap.h"

#include "../../rapidjson/writer.h"
#include "../../rapidjson/reader.h"
#include "../../rapidjson/stringbuffer.h"
#include "../../rapidjson/prettywriter.h"
#include "../../rapidjson/document.h"

VTypeManager::VTypeManager()
{
}

VTypeManager::~VTypeManager()
{
}

void VTypeManager::AddType(VMetaData* pMetaData)
{
	mTypes[pMetaData->GetTypeName()] = pMetaData;
	mTypesByIndex[pMetaData->GetTypeId()] = pMetaData;
}

void VTypeManager::ReflectTypes()
{
	for (auto& type : mTypes)
	{
		if (auto reflector = type.second->GetTypeTable()->ReflectType)
		{
			(*reflector)(*type.second);
		}
	}
}

const VMetaData* VTypeManager::GetMetaData(const int pTypeIndex) const
{
	auto result = mTypesByIndex.find(pTypeIndex);

	if (result != mTypesByIndex.end())
	{
		return result->second;
	}

	return nullptr;
}

const VMetaData* VTypeManager::GetMetaData(const std::string& pTypeName) const
{
	auto result = mTypes.find(pTypeName);
	
	if (result != mTypes.end())
	{
		return result->second;
	}

	return nullptr;
}

std::string VTypeManager::ToString(const Any& pWhat)
{
	if (!pWhat.IsNull())
	{
		return "[" + pWhat.MetaData()->GetTypeName() + "]";
	}

	return "[null]";
}

int VTypeManager::GetTypesCount() const
{
	return mTypes.size();
}

void VTypeManager::EnumerateTypes(std::function<void(const VMetaData*)> pEnumCallback) const
{
	if (pEnumCallback == nullptr)
	{
		return;
	}

	for (auto& type : mTypes)
	{
		pEnumCallback(type.second);
	}
}

bool VTypeManager::CanConvert(const VMetaData* from, const VMetaData* to)
{
	if (from == nullptr || to == nullptr)
		return false;

    if (from == to)
        return true;

	const VMetaData* refMetaData = ::GetMetaData<Any>();

	if (from == refMetaData || to == refMetaData)
		return true;
	
	if (from->IsDerivedFrom(to) || to->IsDerivedFrom(from))
		return true;

	const auto& methods = from->GetMethods();

	for (auto method : methods)
	{
		auto func = method->Method();

		if (func->GetType() == VFunctionType_Static &&
			func->GetArgumentsCount() == 1 &&
			func->GetReturnType()->GetTypeId() == to->GetTypeId() &&
			func->GetArgumentType(0)->GetTypeId() == from->GetTypeId() &&
			func->GetName() == "(cast)")
		{
			return true;
		}
	}

	return false;
}

VBoxedValuePtr VTypeManager::Convert(const Any& what, const VMetaData* to)
{
	auto from = what.MetaData();

	if (from == nullptr)
	{
		return nullptr;
	}

	if (from->IsDerivedFrom(to) || to->IsDerivedFrom(from))
	{
		return what;
	}
	
	const auto& methods = from->GetMethods();

	for (auto method : methods)
	{
		auto func = method->Method();

		if (func->GetType() == VFunctionType_Static &&
			func->GetArgumentsCount() == 1 &&
			func->GetReturnType()->GetTypeId() == to->GetTypeId() &&
			func->GetArgumentType(0)->GetTypeId() == from->GetTypeId() &&
			func->GetName() == "(cast)")
		{
			auto args = { what };
			return func->Invoke(args);
		}
	}

	return nullptr;
}

class SerializerContext
{
public:
	std::map<uint64_t, int> mObjects;

	void WriteObjectIds(rapidjson::PrettyWriter<rapidjson::StringBuffer>* writer)
	{
		writer->Key("MetaData");
		writer->StartObject();

		writer->Key("Objects");
		writer->StartArray();

		for (auto& id : mObjects)
		{
			writer->Uint64(id.first);
		}

		writer->EndArray();
		writer->EndObject();
	}
};

void RecursiveSerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>* writer, Any pWhat, SerializerContext& ctx, VClassMember* pMember = nullptr)
{
	if (pWhat.IsNull())
	{
		writer->Null();
	}
	else
	{
		const auto metaData = pWhat.MetaData();

		if (metaData == GetMetaData<int>() || metaData == GetMetaData<uint16_t>())
		{
			writer->Int(pWhat);
		}		
		else if (metaData == GetMetaData<float>())
		{
			writer->Double(pWhat.As<float>());
		}
		else if (metaData == GetMetaData<bool>())
		{
			writer->Bool(pWhat);
		}
		else if (metaData == GetMetaData<std::string>())
		{
			writer->String(pWhat.As<std::string>().c_str());
		}
		else if (metaData == GetMetaData<std::wstring>())
		{
			writer->String(pWhat.As<std::string>().c_str());
		}
		else
		{
			if (metaData->IsDerivedFrom(GetMetaData<VObject>()))
			{
				auto id = pWhat["InstanceId"].As<uint64_t>();

				if (id > 0)
				{
					ctx.mObjects[id] = 0;
				}
			}

			const auto fieldCount = metaData->GetFieldsCount();
			const auto propertiesCount = metaData->GetPropertiesCount();
			const auto isNoMembersToSave = fieldCount == 0 && propertiesCount == 0;
			
			const auto range = metaData->GetMember("Range");
			const auto pushBack = metaData->GetMember("PushBack");
			const auto at = metaData->GetMember("At");
			const auto size = metaData->GetMember("Size");
			const auto set = metaData->GetMember("Set");
			
			const auto isArray = pushBack && at && size;
			const auto isMap = range && at && size && set;

			V_ASSERT(!(isMap && isArray));

			writer->StartObject();

			writer->Key("Type");
			writer->String(metaData->GetTypeName().c_str());

			if (isNoMembersToSave && !isArray && !isMap)
			{
				const auto asString = pWhat.As<std::string>();

				if (!asString.empty())
				{
					writer->Key("Value");
					writer->String(asString.c_str());
				}
			}
			else
			{
				for (size_t i = 0; i < fieldCount; i++)
				{
					const auto field = metaData->GetField(i);
					const auto& fieldName = field->Name();

					if (pMember != nullptr && field->HasAttribute("SerializeIfOwner") && pMember->HasAttribute("NotOwner"))
					{
						continue;
					}

					writer->Key(fieldName.c_str());
					RecursiveSerializeJson(writer, pWhat[fieldName], ctx, field);
				}

				for (size_t i = 0; i < propertiesCount; i++)
				{
					const auto property = metaData->GetProperty(i);
					const auto& propertyName = property->Name();

					if (pMember != nullptr && property->HasAttribute("SerializeIfOwner") && pMember->HasAttribute("NotOwner"))
					{
						continue;
					}

					writer->Key(propertyName.c_str());
					RecursiveSerializeJson(writer, pWhat[propertyName], ctx);
				}

				if (isArray)
				{
					const int ArraySize = pWhat["Size"]();

					if (ArraySize > 0)
					{
						writer->Key("Elements");
						writer->StartArray();

						for (int j = 0; j < ArraySize; j++)
						{
							RecursiveSerializeJson(writer, pWhat["At"](j), ctx);
						}

						writer->EndArray();
					}
				}
				else if (isMap)
				{
					std::vector<Any> range = pWhat["Range"]();

					if (range.size() > 0)
					{
						writer->Key("Elements");
						writer->StartArray();

						for (size_t j = 0; j < range.size(); j++)
						{
							writer->StartObject();

							writer->Key("Key");
							RecursiveSerializeJson(writer, range[j], ctx);

							writer->Key("Value");
							RecursiveSerializeJson(writer, pWhat["At"](range[j]), ctx);

							writer->EndObject();
						}

						writer->EndArray();
					}
				}
			}

			writer->EndObject();
		}
	}
}

class DeserializerContext
{
public:
	struct MetaDataInfo
	{
		size_t fieldCount;
		size_t propertiesCount;
		bool isNoMembersToLoad;
		VClassMember* range;
		VClassMember* pushBack;
		VClassMember* at;
		VClassMember* size;
		VClassMember* set;
		bool isArray;
		bool isMap;
	};

	struct Ptr
	{
		Any object;
		std::string member;
	};

	std::unordered_map<uint64_t, uint64_t> mObjectsIdMap;
	std::unordered_map<const VMetaData*, MetaDataInfo*> mCache;

	MetaDataInfo& GetInfo(const VMetaData* pMetaData)
	{
		auto result = mCache.find(pMetaData);

		if (result != mCache.end())
		{
			return *result->second;
		}

		auto infoPtr = new MetaDataInfo;
		auto& info = *infoPtr;

		info.fieldCount = pMetaData->GetFieldsCount();
		info.propertiesCount = pMetaData->GetPropertiesCount();
		info.isNoMembersToLoad = info.fieldCount == 0 && info.propertiesCount == 0;

		info.range = pMetaData->GetMember("Range");
		info.pushBack = pMetaData->GetMember("PushBack");
		info.at = pMetaData->GetMember("At");
		info.size = pMetaData->GetMember("Size");
		info.set = pMetaData->GetMember("Set");

		info.isArray = info.pushBack && info.at && info.size;
		info.isMap = info.range && info.at && info.size && info.set;

		mCache[pMetaData] = infoPtr;

		return *infoPtr;
	}

	~DeserializerContext()
	{
		for (auto info : mCache)
		{
			delete info.second;
		}
	}
};

Any RecursiveDeserializeJson(rapidjson::Value& pFrom, DeserializerContext& ctx)
{
	static const std::string sTag = "Deserializer: ";

	if (pFrom.IsNull())
	{
		return nullptr;
	}
	else if (pFrom.IsObject())
	{
		if (!pFrom.HasMember("Type"))
		{
			VLog::Error(sTag, "Object value has no 'Type' member!");
			return nullptr;
		}

		const auto type = pFrom["Type"].GetString();
		const auto metaData = VTypeManager::Instance().GetMetaData(type);

		if (metaData == nullptr)
		{
			VLog::Error(sTag, "Unable to deserialize object of unexisting type: '", type, "'");
			return nullptr;
		}

		auto object = Any(metaData->New());

		if (object.IsNull())
		{
			VLog::Error(sTag, "Unable to create object of type: '", type, "'");
			return nullptr;
		}

		auto& info = ctx.GetInfo(metaData);

		V_ASSERT(!(info.isMap && info.isArray));

		if (info.isNoMembersToLoad && !info.isArray && !info.isMap && pFrom.HasMember("Value"))
		{
			object.Assign(pFrom["Value"].GetString());
		}
		else
		{
			for (auto iter = pFrom.MemberBegin(); iter != pFrom.MemberEnd(); iter++)
			{
				if (iter->value.IsArray())
				{
					continue;
				}

				const auto key = iter->name.GetString();
				const auto member = metaData->GetMember(key);

				if (member == nullptr)
				{
					continue;
				}

				auto value = RecursiveDeserializeJson(iter->value, ctx);
				auto isInstanceId = strcmp(key, "InstanceId") == 0;

				if (isInstanceId)
				{
					if (value.MetaData() == GetMetaData<uint64_t>())
					{
						if (auto obj = object.Cast<VObject>())
						{
							auto& objmap = ObjectManager::Instance().mIDObjectMap;
							auto prev = obj->GetInstanceId();

							objmap.erase(prev);

							auto existingId = ctx.mObjectsIdMap.find(value);

							if (existingId != ctx.mObjectsIdMap.end())
							{
								object[key] = existingId->second;
								objmap.insert(std::make_pair(existingId->second, obj));
							}
							else
							{
								V_ASSERT(false);
							}
						}
					}
				}
				else
				{
					object[key] = value;
				}
			}

			if (pFrom.HasMember("Elements"))
			{
				const auto& elements = pFrom["Elements"].GetArray();
				const auto count = elements.Size();

				if (info.isArray)
				{
					for (rapidjson::SizeType j = 0; j < count; j++)
					{
						auto result = RecursiveDeserializeJson(elements[j], ctx);
						object["PushBack"](result.As(info.at->MetaData()));
					}
				}
				else if (info.isMap)
				{
					for (rapidjson::SizeType j = 0; j < count; j++)
					{
						if (elements[j].HasMember("Key") && elements[j].HasMember("Value"))
						{
							auto key = RecursiveDeserializeJson(elements[j]["Key"], ctx);
							auto value = RecursiveDeserializeJson(elements[j]["Value"], ctx);

							object["Set"](key, value);
						}
					}
				}
			}
		}

		if (metaData->HasAttribute("IdPtr"))
		{
			auto id = object["ObjectId"].As<uint64_t>();
			auto existingId = ctx.mObjectsIdMap.find(id);

			if (existingId != ctx.mObjectsIdMap.end())
			{
				object["ObjectId"] = existingId->second;
			}
			else
			{
				object["ObjectId"].Assign(0ULL);
			}
		}

		auto methods = metaData->GetMethodsByTag("AfterDeserialize");

		if (!methods.empty())
		{
			for (auto method : methods)
			{
				if (method->Type() == VClassMemberType_Method)
				{
					object[method->Name()]();
					break;
				}
			}
		}

		return object;
	}
	else if (pFrom.IsInt())
	{
		return pFrom.GetInt();
	}
	else if (pFrom.IsFloat())
	{
		return pFrom.GetFloat();
	}
	else if (pFrom.IsBool())
	{
		return pFrom.GetBool();
	}

	return pFrom.GetString();
}

std::string VTypeManager::SerializeJson(const Any& pWhat)
{
	if (pWhat.IsNull())
		return std::string();
	
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	writer.SetMaxDecimalPlaces(5);

	writer.StartObject();
	writer.Key("Data");

	SerializerContext ctx;
	RecursiveSerializeJson(&writer, pWhat, ctx);

	ctx.WriteObjectIds(&writer);

	writer.EndObject();
	return buffer.GetString();
}

void RecursiveMigrate(rapidjson::Value& pFrom, rapidjson::MemoryPoolAllocator<>& alloc, bool* migrated)
{
	if (pFrom.IsObject())
	{
		for (auto iter = pFrom.MemberBegin(); iter != pFrom.MemberEnd(); iter++)
		{
			if (iter->name == "Childs" && iter->value.IsArray())
			{
				rapidjson::Value childs;
				childs.SetObject();
				childs.AddMember(rapidjson::Value("Type"), rapidjson::Value("vector<VNode2D*>"), alloc);
				
				rapidjson::Value elements;	
				elements.CopyFrom(iter->value, alloc);

				pFrom.EraseMember(iter);

				for (auto iter1 = elements.Begin(); iter1 != elements.End(); iter1++)
				{
					RecursiveMigrate(*iter1, alloc, migrated);
				}
				
				childs.AddMember(rapidjson::Value("Elements"), elements, alloc);
				pFrom.AddMember(rapidjson::Value("mChilds"), childs, alloc);

				*migrated = true;
				break;
			}

			if (iter->name == "Layers" && iter->value.IsArray())
			{
				rapidjson::Value childs;
				childs.SetObject();
				childs.AddMember(rapidjson::Value("Type"), rapidjson::Value("vector<VAnimationLayer*>"), alloc);

				rapidjson::Value elements;
				elements.CopyFrom(iter->value, alloc);

				pFrom.EraseMember(iter);

				for (auto iter1 = elements.Begin(); iter1 != elements.End(); iter1++)
				{
					RecursiveMigrate(*iter1, alloc, migrated);
				}

				childs.AddMember(rapidjson::Value("Elements"), elements, alloc);
				pFrom.AddMember(rapidjson::Value("mLayers"), childs, alloc);

				*migrated = true;
				break;
			}

			if (iter->name == "Channels" && iter->value.IsArray())
			{
				rapidjson::Value childs;
				childs.SetObject();
				childs.AddMember(rapidjson::Value("Type"), rapidjson::Value("vector<VAnimationChannelBase*>"), alloc);

				rapidjson::Value elements;
				elements.CopyFrom(iter->value, alloc);

				pFrom.EraseMember(iter);

				for (auto iter1 = elements.Begin(); iter1 != elements.End(); iter1++)
				{
					RecursiveMigrate(*iter1, alloc, migrated);
				}

				childs.AddMember(rapidjson::Value("Elements"), elements, alloc);
				pFrom.AddMember(rapidjson::Value("mChannels"), childs, alloc);

				*migrated = true;
				break;
			}

			if (iter->name == "Keys" && iter->value.IsArray())
			{
				rapidjson::Value childs;
				childs.SetObject();
				childs.AddMember(rapidjson::Value("Type"), rapidjson::Value("vector<VAnimationKeyInterface*>"), alloc);

				rapidjson::Value elements;
				elements.CopyFrom(iter->value, alloc);

				pFrom.EraseMember(iter);

				for (auto iter1 = elements.Begin(); iter1 != elements.End(); iter1++)
				{
					RecursiveMigrate(*iter1, alloc, migrated);
				}

				childs.AddMember(rapidjson::Value("Elements"), elements, alloc);
				pFrom.AddMember(rapidjson::Value("mKeys"), childs, alloc);

				*migrated = true;
				break;
			}
		}
	}
}

VBoxedValuePtr VTypeManager::DeserializeJson(const std::string& pJson)
{
	rapidjson::Document doc;
	doc.Parse(pJson.c_str());

	if (!doc.HasParseError())
	{
		rapidjson::Value* data = &doc;		

		DeserializerContext ctx;

		if (doc.IsObject())
		{
			auto dataMember = doc.FindMember("Data");
			
			if (dataMember != doc.MemberEnd())
			{
				data = &dataMember->value;
			}

			auto metaData = doc.FindMember("MetaData");

			if (metaData != doc.MemberEnd())
			{
				auto objects = metaData->value.FindMember("Objects");

				if (objects != metaData->value.MemberEnd())
				{
					V_ASSERT(objects->value.IsArray());

					for (auto it = objects->value.Begin(); it != objects->value.End(); ++it)
					{
						auto oldId = it->GetUint64();
						auto newId = oldId;

						if (ObjectManager::Instance().mIDObjectMap.find(oldId) != ObjectManager::Instance().mIDObjectMap.end())
						{
							newId = ObjectManager::Instance().GenerateID();
						}

						ctx.mObjectsIdMap[oldId] = newId;
					}
				}
			}
		}

		auto migrated = false;
		RecursiveMigrate(*data, doc.GetAllocator(), &migrated);

		auto result = RecursiveDeserializeJson(*data, ctx);
		return result;
	}

	return nullptr;
}

std::string VTypeManager::GetTypeName(int pId)
{
	for (auto type : mTypes)
	{
		if (type.second->mTypeId == pId)
		{
			return type.second->mTypeName;
		}
	}

	return std::string();
}

int VTypeManager::GetTypeId(const std::string& pName)
{
	auto result = mTypes.find(pName);

	if (result != mTypes.end())
	{
		return result->second->mTypeId;
	}

	return -1;
}

bool VTypeManager::IsTypeRegistered(VMetaData* pMetaData) const
{
	for (auto type : mTypes)
	{
		if (type.second->GetTypeName() == pMetaData->GetTypeName())
		{
			return true;
		}
	}

	return false;
}

void VTypeManager::RegisterType(VMetaData* pType)
{	
	if (!IsTypeRegistered(pType))
	{
		/*LOGI("Deffered linking: '%s'\n", pType->GetTypeName().c_str());
		mTypes.erase(pType->GetTypeName());
		mTypesByIndex.erase(pType->GetTypeId());*/

		AddType(pType);
	}
	else
	{
		LOGI("WARNING: Type '%s' is already REGISTERED!\n", pType->GetTypeName().c_str())
	}
}

VTypeManager& VTypeManager::Instance()
{
	static VTypeManager manager;
	return manager;
}