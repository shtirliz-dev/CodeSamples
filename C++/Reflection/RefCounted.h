#pragma once

#include "Reflection/Ptr.h"

class VRefCounted
{
private:
	int mReferencesCount;

protected:
	virtual void FreeAsNotReferenced() = 0;

public:
	inline VRefCounted() : mReferencesCount(0)
	{
	}

	inline int GetReferencesCount() const
	{
		return mReferencesCount;
	}

	inline void Reference()
	{
		++mReferencesCount;
	}

	inline void Dereference()
	{
		V_ASSERT(mReferencesCount > 0);

		if (!--mReferencesCount)
		{
			FreeAsNotReferenced();
		}
	}

protected:
	VRefCounted(const VRefCounted&) = delete;
	VRefCounted(VRefCounted&&) = delete;
};