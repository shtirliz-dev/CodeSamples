#pragma once

#include "Reflection/RefCounted.h"
#include "Scripting/Lua.h"
#include <cstddef>
#include "sparsehash/dense_hash_map"

class VMetaData;
class VObject;

template<typename T>
class VTypeInfo;

class ObjectManager
{
public:
	uint64_t mID;
	google::dense_hash_map<uint64_t, VObject*> mIDObjectMap;

	uint64_t GenerateID();
	uint64_t Register(VObject* object);
	void Unregister(VObject* object);
	void Unregister(uint64_t object);

	static ObjectManager& Instance();
};

class IdLink
{
public:
	uint64_t mId = -1;

	IdLink();

	IdLink(VObject* obj);
	IdLink(uint64_t obj);

	VObject* Get() const;
	void Set(VObject* obj);

	VObject* operator->();

	static void ReflectType(VMetaData& pMeta);
	static void RegisterTypeLua(VLuaState* pLua);
};

class VObject : public VRefCounted
{
private:
	uint64_t mInstanceId;

protected:
	static VTypeInfo<VObject>* TypeInfo;

protected:
	void FreeAsNotReferenced() override;

public:
	VObject();
	virtual ~VObject();

	virtual const VMetaData* MetaData() const;
	ptr<VObject> Share() const;

	static void* operator new(size_t size);
	static void operator delete(void* data);

	static void ReflectType(VMetaData& pMeta);
	static void RegisterTypeLua(VLuaState* pLua);

	uint64_t GetInstanceId() const;
	void SetInstanceId(uint64_t id);

	friend class ObjectManager;
};

#ifdef _DEBUG
# define SIDENS2(x) #x
# define SIDENS(x) SIDENS2(x)
# define NEW(...) ObjectSetAllocationInfo(new __VA_ARGS__, #__VA_ARGS__ "  " __FILE__ "(" SIDENS(__LINE__) ")")
# define NEW_WITH_TAG(tag, ...) ObjectSetAllocationInfo(new __VA_ARGS__, "[" tag "] " #__VA_ARGS__ "  " __FILE__ "(" SIDENS(__LINE__) ")")

void ManagedHeapSetAllocationInfo(void*, const char* info);

template <typename T>
T* ObjectSetAllocationInfo(T* data, const char* info)
{
	ManagedHeapSetAllocationInfo(data, info);
	return data;
}

#else
# define NEW(...) new __VA_ARGS__
# define NEW_WITH_TAG(tag, ...) new __VA_ARGS__
#endif
