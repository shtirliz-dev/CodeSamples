#include "Utils/ManagedHeap.h"
#include "Reflection/Object.h"
#include "Reflection/TypeManager.h"
#include "Reflection/FunctionWrap.h"
#include "xxhash/xxhash.h"
#include <random>
#include "Reflection/Attribute.h"

IdLink::IdLink()
{
}

IdLink::IdLink(VObject* obj)
{
	mId = obj->GetInstanceId();
}

IdLink::IdLink(uint64_t obj)
{
	mId = obj;
}

VObject* IdLink::Get() const
{
	auto& objects = ObjectManager::Instance().mIDObjectMap;
	auto object = objects.find(mId);

	if (object != objects.end())
	{
		return object->second;
	}

	return nullptr;
}

void IdLink::Set(VObject* obj)
{
	mId = obj->GetInstanceId();
}

VObject* IdLink::operator->()
{
	return Get();
}

std::string IdLinkToString(IdLink* link)
{
	if (auto object = link->Get())
	{
		return "[" + object->MetaData()->GetTypeName() + "]";
	}

	return "[null]";
}

void IdLink::ReflectType(VMetaData& pMeta)
{
	pMeta.SetAttributes(VAttribute("IdPtr"));
	pMeta.AddConstructor<IdLink()>();
	pMeta.AddField("ObjectId", &IdLink::mId);
	pMeta.AddMethod("Set", &IdLink::Set);
	pMeta.AddStaticFunction("(cast)", &IdLinkToString);
	pMeta.AddStaticFunction("RegisterTypeLua", &IdLink::RegisterTypeLua);
}

void IdLink::RegisterTypeLua(VLuaState* pLua)
{
	auto get = [](IdLink& thiz) -> sol::object
	{
		if (auto object = thiz.Get())
		{
			return object->MetaData()->GetLuaObject(VLua::Instance()->GetState(), object);
		}

		return sol::lua_nil;
	};

	auto index = [=](IdLink& thiz, const std::string& what) -> sol::object
	{
		if (auto object = thiz.Get())
		{
			auto target = Any(object)[what];

			if (!target.IsNull())
			{
				return target.MetaData()->GetLuaObject(VLua::Instance()->GetState(), target.GetValue()->GetData(target.MetaData()));
			}
		}

		return sol::lua_nil;
	};

	pLua->new_usertype<IdLink>("IdLink",
		"Get", get,
		sol::meta_function::index, index);
}

REGISTER_TYPE(IdLink);

uint64_t ObjectManager::GenerateID()
{
	static uint64_t t = time(nullptr);
	static uint64_t s = t * t;
	static std::random_device rd;
	static std::mt19937_64 gen(rd());
	static std::uniform_int_distribution<uint64_t> dis;

	auto r = dis(gen);
	uint64_t st[] = { r, ++mID, t };
	auto h = XXH64(st, sizeof(st), s);

	V_ASSERT(
		h != 0ULL && 
		h != -1ULL && 
		h != -2ULL
	);

	return h;
}

uint64_t ObjectManager::Register(VObject* object)
{
	auto h = GenerateID();
	mIDObjectMap.insert(std::make_pair(h, object));
	object->SetInstanceId(h);
	return h;
}

void ObjectManager::Unregister(VObject* object)
{
	Unregister(object->GetInstanceId());
}

void ObjectManager::Unregister(uint64_t object)
{
	mIDObjectMap.erase(object);
}

ObjectManager& ObjectManager::Instance()
{
	static ObjectManager* sInstance = nullptr;

	if (sInstance == nullptr)
	{
		sInstance = new ObjectManager();
		sInstance->mID = 0;
		sInstance->mIDObjectMap.set_empty_key(-1);
		sInstance->mIDObjectMap.set_deleted_key(-2);
	}

	return *sInstance;
}

const VMetaData* VObject::MetaData() const
{
	return GetMetaData<VObject>(); 
}

ptr<VObject> VObject::Share() const
{ 
	return const_cast<VObject*>(this); 
}

VObject::VObject():
	mInstanceId(0)
{
	ObjectManager::Instance().Register(this);
}

VObject::~VObject()
{
	V_ASSERT(GetReferencesCount() == 0);
	ObjectManager::Instance().Unregister(this);
}

void VObject::FreeAsNotReferenced()
{
	V_ASSERT(GetReferencesCount() == 0);
	delete this;
}

uint64_t VObject::GetInstanceId() const
{
	return mInstanceId;
}

void VObject::SetInstanceId(uint64_t id)
{
	mInstanceId = id;
}

void* VObject::operator new(size_t size)
{
	//auto result = GetManagedHeap()->Allocate(size);
	//VGarbageCollector::Instance().Release(static_cast<VObject*>(result));

#ifdef WIN32
	return _aligned_malloc(size, 16);
#else
	return malloc(size);
#endif
}

void VObject::operator delete(void* data)
{
	//VGarbageCollector::Instance().Forget(static_cast<VObject*>(data));
	//return GetManagedHeap()->Free(data);

#ifdef WIN32
	_aligned_free(data);
#else
	free(data);
#endif
}

void VObject::ReflectType(VMetaData& pMeta)
{
	pMeta.AddStaticFunction("RegisterTypeLua", &VObject::RegisterTypeLua);
	pMeta.AddProperty("InstanceId", &VObject::GetInstanceId, &VObject::SetInstanceId, VAttribute("Hidden"));
}

void VObject::RegisterTypeLua(VLuaState* pLua)
{
	pLua->new_usertype<VObject>("VObject",
		"Reference", &VObject::Reference,
		"Dereference", &VObject::Dereference,
		"GetReferencesCount", &VObject::GetReferencesCount,
		"MetaData", &VObject::MetaData);
}

REGISTER_TYPE(VObject);