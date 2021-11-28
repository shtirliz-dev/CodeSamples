#pragma once

class VMetaData;

#include <Reflection/Any.h>
#include <functional>

enum VFunctionType
{
	VFunctionType_Unknown,
	VFunctionType_ClassMethod,
	VFunctionType_Static,
	VFunctionType_Constructor,
	VFunctionType_CopyConstructor,
	VFunctionType_OperatorAssigment,
    VFunctionType_ClassMethodExternal,
};

class VFunction
{
protected:
	VMetaData* mRetType = nullptr;
	VMetaData* mOwner = nullptr;
	std::vector<VMetaData*> mArgTypes;
	VFunctionType mType = VFunctionType_Unknown;
	std::string mName;

	virtual Any InvokeStatic(Any* args);
	virtual Any InvokeMethod(void* obj, Any* args);

public:
	VFunction(VFunctionType pType);

	int GetArgumentsCount() const;

	VMetaData* GetArgumentType(int index) const;
	VMetaData* GetReturnType() const;
	VMetaData* GetParentType() const;

	void ToString();

	std::string GetName() const;
	VFunctionType GetType() const;

	Any Invoke();
	Any Invoke(const std::vector<Any>& args);
	Any InvokeStaticUnsafe(Any* args);

	Any Invoke(void* obj);
	Any Invoke(void* obj, const std::vector<Any>& args);
	Any InvokeMethodUnsafe(void* obj, Any* args);
};

template<typename T, typename... Args>
Any Unpack(const T& what, const Args&... args);

template<typename T, typename... Args>
Any Unpack(const T& what, const Args&... args)
{
    return Any(what);
}

template<typename T, typename... Args>
const T& UnpackW(const T& what, const Args&... args)
{
    return what;
}