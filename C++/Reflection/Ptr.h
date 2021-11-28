#pragma once

#include <Core/CodeConfig.h>
#include <Core/Assert.h>
#include <Core/CommonTypes.h>

#if defined(_DEBUG) && defined(_M_IX86)
# define CHECK_OBJECT() V_ASSERT(mObject != nullptr)
#else
# define CHECK_OBJECT()
#endif

template <typename T>
class ptr
{
private:
	struct Dereferencer
	{
		static void Dereference(T* instance)
		{
			instance->Dereference();
		}
	};

	using DereferenceDelegate = void(*)(T*);

	T* _object;
	DereferenceDelegate _decRef;

private:
	inline void Assign(T* object)
	{
		_object = object;

		if (_object)
		{
			_object->Reference();
			_decRef = &Dereferencer::Dereference;
		}
	}

	inline void Exchange(T* object)
	{
		auto prevObject = _object;
		auto prevDecRef = _decRef;

		Assign(object);

		if (prevObject)
		{
			prevDecRef(prevObject);
		}
	}

public:
	inline ptr() :
		_object(nullptr),
		_decRef(nullptr)
	{
	}

	inline ptr(const nullptr_t& pPointer)
	{
		Assign(nullptr);
	}

	inline ptr(T* object)
	{
		Assign(object);
	}

	inline ptr(const ptr<T>& other)
	{
		Assign(other._object);
	}

	inline ptr(ptr<T>&& other)
	{
		_object = other._object;
		_decRef = other._decRef;

		other._object = nullptr;
		other._decRef = nullptr;
	}

	template <typename TT>
	inline ptr(const ptr<TT>& other)
	{
		Assign(static_cast<TT*>(other));
	}

	inline ~ptr()
	{
		if (_object)
		{
			_decRef(_object);
		}
	}

	inline void operator=(const ptr<T>& other)
	{
		Exchange(other._object);
	}

	inline void operator=(ptr<T>&& other)
	{
		_object = other._object;
		_decRef = other._decRef;

		other._object = nullptr;
		other._decRef = nullptr;
	}

	template <typename TT>
	inline void operator=(const ptr<TT>& other)
	{
		Exchange(static_cast<TT*>(other));
	}

	inline T* operator->() const
	{
		CHECK_OBJECT();
		return _object;
	}

	inline operator T*() const
	{
		return _object;
	}

	inline operator bool() const
	{
		return !!_object;
	}

	inline bool operator==(const ptr<T>& other) const
	{
		return _object == other._object;
	}

	inline bool operator==(const T* other) const
	{
		return _object == other;
	}

	inline bool operator==(T* other) const
	{
		return _object == other;
	}

	inline bool operator!=(const ptr<T>& other) const
	{
		return _object != other._object;
	}

	inline bool operator==(const nullptr_t& other) const
	{
		return _object == other;
	}

	inline bool operator!=(const nullptr_t& other) const
	{
		return _object != other;
	}

	T* Get() const
	{
		return _object;
	}

	template <typename TT>
	ptr<TT> StaticCast() const
	{
		return static_cast<TT*>(_object);
	}

	template <typename TT>
	ptr<TT> DynamicCast() const
	{
		return dynamic_cast<TT*>(_object);
	}

	template <typename TT>
	ptr<TT> FastCast() const
	{
#ifdef _DEBUG
		return dynamic_cast<TT*>(_object);
#else
		return static_cast<TT*>(_object);
#endif
	}
};

template <typename T>
inline ptr<T> MakePointer(T* p)
{
	return p;
}

#undef CHECK_OBJECT

namespace std
{
	template <typename T>
	struct hash<ptr<T>>
	{
		size_t operator()(const ptr<T>& p) const
		{
			return reinterpret_cast<size_t>(static_cast<T*>(p));
		}
	};
}