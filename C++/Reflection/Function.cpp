#include <iostream>
#include "Reflection/Function.h"
#include "Reflection/MetaData.h"

VFunction::VFunction(VFunctionType pType) : mType(pType)
{ }

Any VFunction::InvokeStatic(Any* args)
{
	return Any();
}

Any VFunction::InvokeMethod(void* obj, Any* args)
{
	return Any();
}

int VFunction::GetArgumentsCount() const
{
	return mArgTypes.size();
}

VMetaData* VFunction::GetArgumentType(int index) const
{
	if (index >= 0 && index < mArgTypes.size())
		return mArgTypes[index];
	return 0;
}

VMetaData* VFunction::GetReturnType() const
{
	return mRetType;
}

VMetaData* VFunction::GetParentType() const
{
	return mOwner;
}

std::string VFunction::GetName() const
{
	return mName;
}

VFunctionType VFunction::GetType() const
{
	return mType;
}

Any VFunction::Invoke(const std::vector<Any>& args)
{
	return InvokeStatic((Any*)&args[0]);
}

Any VFunction::Invoke()
{
    return InvokeStatic(nullptr);
}

Any VFunction::Invoke(void* obj, const std::vector<Any>& args)
{
	return InvokeMethod(obj, (Any*)&args[0]);
}

Any VFunction::InvokeStaticUnsafe(Any* args)
{
    return InvokeStatic(args);
}

Any VFunction::InvokeMethodUnsafe(void* obj, Any* args)
{
    return InvokeMethod(obj, args);
}

Any VFunction::Invoke(void* obj)
{
	return InvokeMethod(obj, nullptr);
}

void VFunction::ToString()
{
	using namespace std;

	switch (mType)
	{
	case VFunctionType_Unknown:
		cout << "Unknown function!" << endl;
		break;
	case VFunctionType_ClassMethod:
	{
		std::string retType = mRetType ? mRetType->GetTypeName() : "void";

		cout << "[class_method] { " << retType << " " << mOwner->GetTypeName() << "::" << mName << "(";

		for (int i = 0; i < mArgTypes.size(); i++)
		{
			if (i != mArgTypes.size() - 1)
				cout << mArgTypes[i]->GetTypeName() << ", ";
			else
				cout << mArgTypes[i]->GetTypeName();
		}

		cout << ") }" << endl;
	}
	break;
	case VFunctionType_Static:
	{
		cout << "[static_function] { " << mRetType->GetTypeName() << " ";

		if (mOwner != 0)
			cout << mOwner->GetTypeName() << "::" << mName << "(";
		else
			cout << mName << "(";

		for (int i = 0; i < mArgTypes.size(); i++)
		{
			if (i != mArgTypes.size() - 1)
				cout << mArgTypes[i]->GetTypeName() << ", ";
			else
				cout << mArgTypes[i]->GetTypeName();
		}

		cout << ") }" << endl;
	}
	break;
	case VFunctionType_Constructor:
	{
		cout << "[class_ctor] { " << mOwner->GetTypeName() << "::" << mName << "(";

		for (int i = 0; i < mArgTypes.size(); i++)
		{
			if (i != mArgTypes.size() - 1)
				cout << mArgTypes[i]->GetTypeName() << ", ";
			else
				cout << mArgTypes[i]->GetTypeName();
		}

		cout << ") }" << endl;
	}
	break;
	default:
		break;
	}
}