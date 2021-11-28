#pragma once

#include <Reflection/Any.h>

struct VAttribute
{
	std::string Name;
	Any Value;

	VAttribute(const std::string& pName) : Name(pName)
	{
	}

	VAttribute(const std::string& pName, const Any& pValue) : Name(pName), Value(pValue)
	{
	}

	operator std::pair<std::string, Any>() const
	{
		return std::make_pair(Name, Value);
	}
};

template <typename T>
std::pair<std::string, Any> UnpackAttribute(const T& pAttr)
{
	return pAttr;
}