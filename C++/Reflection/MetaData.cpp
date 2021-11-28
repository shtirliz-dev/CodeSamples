#include <iostream>
#include "Reflection/MetaData.h"
#include "Reflection/MetaField.h"
#include "Reflection/TypeManager.h"
#include "Containers/Array.h"
#include "Reflection/MetaProperty.h"
#include "Reflection/MetaMethod.h"
#include "Reflection/Function.h"
#include "Reflection/FunctionWrap.h"

size_t VMetaData::mTypeIdCounter = 0;

VMetaMethod* VFuncArgumentMatchFinder::Result()
{
	if (mResult)
		return mResult;

	switch (mAcceptable.size())
	{
	case 0: return 0;
	case 1: return mAcceptable.front();
	default:
		throw std::runtime_error("Few function signatures match given argument list. Ambiguous call!");
	}

	return 0;
}

VFuncArgumentMatchFinder::VFuncArgumentMatchFinder(const std::vector<VMetaMethod*>& pFunctions, const std::string& pName, const std::vector<VMetaData*>& pGivenArguments)
{
	for (auto funcIter : pFunctions)
	{
		auto func = funcIter->Method();

		if (!func->GetName().compare(pName) && func->GetArgumentsCount() == pGivenArguments.size())
		{
			bool match = true;
			bool exactMatch = true;

			for (int i = 0; i < func->GetArgumentsCount() && match; i++)
			{
				VMetaData* meta = func->GetArgumentType(i);

				if (pGivenArguments[i]->GetTypeId() == meta->GetTypeId())
					continue;

				if (VTypeManager::Instance().CanConvert(pGivenArguments[i], meta))
				{
					exactMatch = false;
				}
				else
				{
					match = false;
					break;
				}
			}

			if (match && exactMatch)
			{
				mResult = funcIter;
				break;
			}
			else if (match && !exactMatch)
				mAcceptable.push_back(funcIter);
		}
	}
}

VFuncArgumentMatchFinder::VFuncArgumentMatchFinder(const std::vector<VMetaMethod*>& pFunctions, const std::string& pName, const std::vector<Any>& pGivenArguments)
{
	for (auto funcIter : pFunctions)
	{
		auto func = funcIter->Method();

		if (!func->GetName().compare(pName) && func->GetArgumentsCount() == pGivenArguments.size())
		{
			bool match = true;
			bool exactMatch = true;

			for (int i = 0; i < func->GetArgumentsCount() && match; i++)
			{
				VMetaData* meta = func->GetArgumentType(i);

				if (pGivenArguments[i].IsNull())
				{
					exactMatch = false;
					continue;
				}

				if (pGivenArguments[i].MetaData()->GetTypeId() == meta->GetTypeId())
					continue;

				if (VTypeManager::Instance().CanConvert(pGivenArguments[i].MetaData(), meta))
				{
					exactMatch = false;
				}
				else
				{
					match = false;
					break;
				}
			}

			if (match && exactMatch)
			{
				mResult = funcIter;
				break;
			}
			else if (match && !exactMatch)
				mAcceptable.push_back(funcIter);
		}
	}
}

VMetaData& VMetaData::SetName(const std::string& name)
{
	mTypeName = name;
	return *this;
}

ptr<VBoxedValue> VMetaData::New() const
{
	if (mDefaultConstructor == nullptr)
	{
		for (auto func : mMethods)
		{
			auto method = func->Method();

			if (method->GetType() == VFunctionType_Constructor &&
				method->GetArgumentsCount() == 0)
			{
				mDefaultConstructor = method;
				break;
			}
		}
	}

	if (mDefaultConstructor)
	{
		return mDefaultConstructor->InvokeStaticUnsafe(nullptr);
	}

	return nullptr;
}

RawPtr VMetaData::Copy(RawPtr pOther) const
{
	if (mCopyConstructor == nullptr)
	{
		for (auto method : mMethods)
		{
			if (method->Method()->GetType() == VFunctionType_CopyConstructor)
			{
				mCopyConstructor = method->Method();
				break;
			}
		}
	}

	throw std::exception();
	//return mCopyConstructor->InvokeStaticUnsafe(&tempRef);
}

void VMetaData::Move(RawPtr pWhere, RawPtr pWhat) const
{
	if (mAssigmentOperator == nullptr)
	{
		for (auto method : mMethods)
		{
			if (method->Method()->GetType() == VFunctionType_OperatorAssigment)
			{
				mAssigmentOperator = method->Method();
				break;
			}
		}
	}
	
	VBoxedHolder holderWhere(pWhere, this);
	VBoxedHolder holderWhat(pWhat, this);
			
	Any args[2] =
	{ 
		Any(&holderWhere),
		Any(&holderWhat)
	};

	mAssigmentOperator->InvokeStaticUnsafe(args);
}

void VMetaData::Delete(RawPtr pOther) const
{
	GetTypeTable()->StaticDelete(&pOther);
}

bool VMetaData::IsManagedType() const
{
	return IsDerivedFrom(::GetMetaData<VObject>()) != nullptr;
}

bool VMetaData::HasMethod(const std::string& name) const
{
	for (auto method : mMethods)
	{
		if (!method->Name().compare(name))
		{
			return true;
		}
	}

    for (auto baseClass : mBaseClasses)
    {
        if (baseClass->BaseMetaData()->HasMethod(name))
            return true;
    }

	return false;
}

void VMetaData::Init(const std::string& pTypeName, size_t pSizeOf)
{
	if (IsInitialized())
	{
		V_ASSERT(false);
		return;
	}

	mTypeName = pTypeName;
	mTypeIdName = pTypeName;
	mSizeOf = pSizeOf;
	mTypeTable = std::make_unique<VTypeTable>();
	mTypeId = ++mTypeIdCounter;
}

VTypeTable* VMetaData::GetTypeTable() const
{ 
	return mTypeTable.get(); 
}

VMetaData::VMetaData()
{
	mTypeTable = nullptr;
}

VMetaData::~VMetaData()
{
}

bool VMetaData::IsInitialized() const
{
	return mTypeId > 0;
}

size_t VMetaData::GetTypeId() const
{
	return mTypeId;
}

size_t VMetaData::GetMethodsCount() const
{
	return mMethods.size();
}

const std::string& VMetaData::GetTypeName() const
{
	return mTypeName;
}

const std::string& VMetaData::GetTypeIdName() const
{
	return mTypeIdName;
}

RawPtr VMetaData::CastPointer(RawPtr pPointer, const VMetaData* pTargetType) const
{
	if (pTargetType != this)
	{
		if (auto helper = pTargetType->IsDerivedFrom(this))
		{
			return helper->BaseToDerived(pPointer);
		}
		else if (auto helper = IsDerivedFrom(pTargetType))
		{
			return helper->DerivedToBase(pPointer);
		}
	}

	return pPointer;
}

bool VMetaData::InClassHierarchy() const
{
	if (mBaseClasses.size() > 0)
		return true;
	
	VMetaData* target = nullptr;

	VTypeManager::Instance().EnumerateTypes([&](const VMetaData* metaData) 
	{
		if (target != nullptr)
		{
			return;
		}

		if (metaData->IsDerivedFrom(this))
		{
			target = const_cast<VMetaData*>(metaData);
		}
	});
	
	return target != nullptr;
}

VMetaData& VMetaData::AddElement(VClassMember* pElement)
{
	if (pElement->Type() != VClassMemberType_None)
	{
		switch (pElement->Type())
		{
		case VClassMemberType_Field:
			mFields.push_back(pElement);
			break;
		case VClassMemberType_Property:
			mProperties.push_back((VMetaProperty*)pElement);
			break;
		case VClassMemberType_Method:
			mMethods.push_back((VMetaMethod*)pElement);
			break;
		}

		mMembers.push_back(pElement);
		mMergedMembers.push_back(pElement);
	}

	return *this;
}

VMetaData& VMetaData::AddElement(VInheritanceHelperBase* pBaseClass)
{
	mBaseClasses.push_back(pBaseClass);
	return *this;
}

void VMetaData::SetMetaDataGetter(VFunction* pMethod)
{
	mMetaDataGetter = pMethod;
}

std::vector<VMetaMethod*> VMetaData::GetMethodsByTag(const std::string& pTag) const
{
    std::vector<VMetaMethod*> result;

    for (auto base : mBaseClasses)
    {
        auto baseProps = base->BaseMetaData()->GetMethodsByTag(pTag);

        if (baseProps.size() >= 0)
        {
            result.insert(result.end(), baseProps.begin(), baseProps.end());
        }
    }

    for (auto prop : mMethods)
    {
        if (prop->HasAttribute(pTag))
        {
            result.push_back(prop);
        }
    }

    return result;
}

std::vector<VMetaProperty*> VMetaData::GetPropertiesByTag(const std::string& pTag) const
{
	std::vector<VMetaProperty*> result;

	for (auto base : mBaseClasses)
	{
		auto baseProps = base->BaseMetaData()->GetPropertiesByTag(pTag);

		if (baseProps.size() >= 0)
		{
			result.insert(result.end(), baseProps.begin(), baseProps.end());
		}
	}

	for (auto prop : mProperties)
	{
		if (prop->HasAttribute(pTag))
		{
			result.push_back(prop);
		}
	}

	return result;
}

bool VMetaData::HasAttribute(const std::string& pAttribute) const
{
	return mAttributes.find(pAttribute) != mAttributes.end();
}

const Any& VMetaData::GetAttribute(const std::string& pAttribute) const
{
	const auto result = mAttributes.find(pAttribute);

	if (result != mAttributes.end())
	{
		return result->second;
	}

	THROW("Type " + mTypeName + " has no attribute with name: " + pAttribute);
}

VMetaData* VMetaData::GetMetaData(void* object)
{
	if (this == ::GetMetaData<VMetaData*>())
	{
		return this;
	}

	if (mMetaDataGetter)
	{
		return mMetaDataGetter->Invoke(object);
	}

	return this;
}

VLuaObject VMetaData::GetLuaObject(lua_State* state, RawPtr object) const
{
	if (state == nullptr || object == nullptr)
	{
		return sol::lua_nil;
	}

	return mTypeTable->GetLuaObject(state, object);
}

size_t VMetaData::GetMembersCount() const
{
	size_t counter = 0;

	for (auto base : mBaseClasses)
	{
		counter += base->BaseMetaData()->GetMembersCount();
	}

	return mMembers.size() + counter;
}

size_t VMetaData::GetFieldsCount() const
{
	size_t counter = 0;

	for (auto base : mBaseClasses)
	{
		counter += base->BaseMetaData()->GetFieldsCount();
	}

	return mFields.size() + counter;
}

size_t VMetaData::GetSizeOf() const
{ 
	return mSizeOf; 
}

size_t VMetaData::GetPropertiesCount() const
{
	size_t counter = 0;

	for (auto base : mBaseClasses)
	{
		counter += base->BaseMetaData()->GetPropertiesCount();
	}

	return mProperties.size() + counter;
}

VInheritanceHelperBase* VMetaData::IsDerivedFrom(const VMetaData* pMetaData) const
{
	if (mBaseClasses.empty())
	{
		return nullptr;
	}

	for (auto base : mBaseClasses)
	{
		auto baseMeta = base->BaseMetaData();

		if (baseMeta == pMetaData)
		{
			return base;
		}

		if (auto res = baseMeta->IsDerivedFrom(pMetaData))
		{
			return res;
		}
	}

	return nullptr;
}

const std::vector<VMetaMethod*>& VMetaData::GetMethods() const
{
	return mMethods;
}

const std::vector<VClassMember*>& VMetaData::GetFields() const
{
	return mFields;
}

const std::vector<VMetaProperty*>& VMetaData::GetProperties() const
{
	return mProperties;
}

const std::vector<VClassMember*>& VMetaData::GetMembers() const
{
	return mMembers;
}

VClassMember* VMetaData::GetMember(size_t index) const
{
	size_t counter = 0;

	for (auto base : mBaseClasses)
	{
		auto baseMeta = base->BaseMetaData();

		if (auto member = baseMeta->GetMember(index))
		{
			return member;
		}
		else
		{
			counter += baseMeta->GetMembersCount();
		}
	}

	index -= counter;

	if (index >= 0 && index < mMembers.size())
	{
		return mMembers[index];
	}

	return nullptr;
}

VMetaField* VMetaData::GetField(size_t index) const
{
	int ctr = 0;

	for (auto base : mBaseClasses)
	{
		if (auto field = base->BaseMetaData()->GetField(index))
			return field;
		else
			ctr += base->BaseMetaData()->GetFieldsCount();
	}

	index -= ctr;

	if (index >= 0 && index < mFields.size())
	{
		return static_cast<VMetaField*>(mFields[index]);
	}

	return 0;
}

VMetaProperty* VMetaData::GetProperty(size_t index) const
{
	int ctr = 0;

	for (auto base : mBaseClasses)
	{
		if (auto field = base->BaseMetaData()->GetProperty(index))
			return field;
		else
			ctr += base->BaseMetaData()->GetPropertiesCount();
	}

	index -= ctr;

	if (index >= 0 && index < mProperties.size())
	{
		return mProperties[index];
	}

	return 0;
}

VClassMember* VMetaData::GetMember(const std::string& pName) const
{
	for (auto member : mMembers)
	{
		if (!member->Name().compare(pName))
		{
			return member;
		}
	}

	if (mBaseClasses.size() > 0)
	{
		for (auto base : mBaseClasses)
		{
			if (auto member = base->BaseMetaData()->GetMember(pName))
			{
				return member;
			}
		}
	}

	return nullptr;
}

void VMetaData::ShowDebug()
{
	std::cout << "[" << mTypeName << "]" << std::endl;

	for (auto field : mFields)
	{
		//cout << "[class_field] { " << field->MetaData()->mTypeName << (field->IsPointer() ? "*" : "") << " " << field->Name() << " }" << endl;
	}

	for (auto method : mMethods)
	{
		method->Method()->ToString();
	}
}

void VTypeRegistrationHelper::RegisterType(VMetaData* pMetaData)
{
	VTypeManager::Instance().RegisterType(pMetaData);

}