#include "Nodes/Node2D.h"
#include "Render/RenderInterface.h"
#include "Reflection/ScriptingEngine.h"
#include "Animation/NodeChannelConnector.h"
#include "Reflection/FunctionWrap.h"
#include "Reflection/Attribute.h"
#include "Actions/Actions.h"
#include "Actions/ActionManager.h"
#include "Scripting/Lua.h"
#include "Scene/Scene.h"

VNode2D::VNode2D() :
	mPosition(0.0f),
	mRotation(0.0f),
	mAnchor(0.0f),
	mSize(1.0f),
	mLastSize(1.0f),
	mScale(1.0f),
	mWorldMatrix(mat4x4::IDENTITY),
	mParent(nullptr),
	mParentScene(nullptr),
	mColor(1, 1, 1, 1),
	mDisplayedColor(1, 1, 1, 1),
	mNeedUpdateMatrix(true),
	mNeedUpdateInverseMatrix(true),
	mIsAnchorPercent(false),
	mIsPositionPercent(false),
	mIsSizePercent(false),
	mVisible(true),
	mIsPaused(false),
	mSelected(false),
	mNeedUpdateColor(true),
	mIsStarted(false),
	mTag(-1),
	mActionHandle(nullptr),
	mCullingEnabled(true),
	mCascadeOpacityEnabled(true),
	mUseAutoBlendMode(true)
{
	mComponents = new VComponentSet();
	mComponents->Init(this);
    mBlendMode = Core::RenderBlendMode::Normal;
}

void VNode2D::Cleanup()
{
	V_ASSERT(!mIsStarted);

	mComponents->Cleanup();
	DeleteChilds();
	mParent = nullptr;
}

VNode2D::~VNode2D()
{
	StopActions();

	if (mActionHandle)
	{
		mActionHandle->SetTarget(nullptr);
	}

	Cleanup();
}

bool VNode2D::IsCanAddComponent(const std::string& pComponentName) const
{
    return mComponents->IsCanAdd(pComponentName);
}

VComponent* VNode2D::GetComponent(const std::string& pName)
{
	return mComponents->Get(pName);
}

std::vector<VComponent*> VNode2D::GetComponents(const std::string& pName)
{
	return mComponents->GetAll(pName);
}

void VNode2D::AddComponent(VComponent* pComponent)
{
    if (pComponent == nullptr)
        return;

	mComponents->Add(pComponent);
}

void VNode2D::RemoveComponent(VComponent* pComponent)
{
    if (pComponent == nullptr)
        return;

	mComponents->Remove(pComponent);
}

void VNode2D::RemoveComponent(const std::string& pName)
{
	mComponents->Remove(mComponents->Get(pName));
}

const std::vector<VComponent*>& VNode2D::GetAllComponents()
{
    return mComponents->GetAll();
}

void VNode2D::RemoveAllComponents()
{
	mComponents->Cleanup();
}

bool VNode2D::IsPaused() const
{
	return mIsPaused;
}

void VNode2D::OnResume()
{
	V_ASSERT(mIsPaused);

	mIsPaused = false;
	mComponents->OnResume();
}

void VNode2D::OnPause()
{
	V_ASSERT(!mIsPaused);

	mComponents->OnPause();
	mIsPaused = true;
}

bool VNode2D::IsStarted() const
{
	return mIsStarted;
}

void VNode2D::OnStart()
{
	V_ASSERT(!mIsStarted);

	mIsStarted = true;

	for (auto child : mChilds)
	{
		child->OnStart();
	}

	mComponents->OnStart();
}

void VNode2D::OnStop()
{
	V_ASSERT(mIsStarted);

	mComponents->OnStop();

	for (auto child : mChilds)
	{
		child->OnStop();
	}

	mIsStarted = false;
}

bool VNode2D::IsCullingEnabled() const
{
	return mCullingEnabled;
}

void VNode2D::SetCullingEnabled(bool pValue)
{
	mCullingEnabled = pValue;
}

VNodeChannelConnectorBase* VNode2D::NewChannelConnector(VAnimationChannelBase* pChannel)
{
	VNodeChannelConnectorBase* result = nullptr;

	auto pChannelName = pChannel->GetName();    
    auto animProps = MetaData()->GetPropertiesByTag("CanBeAnimated");

    for (auto prop : animProps)
    {
        if (prop->Name() == pChannelName)
        {
            result = new VNodeChannelConnectorUniversal(this, pChannel, prop->Getter(), prop->Setter());
            break;
        }
    }

	for (auto component : mComponents->GetAll())
	{
		animProps = component->MetaData()->GetPropertiesByTag("CanBeAnimated");

		for (auto prop : animProps)
		{
			if ((component->GetName() + "." + prop->Name()) == pChannelName)
			{
				result = new VNodeChannelConnectorUniversal(component, pChannel, prop->Getter(), prop->Setter());
				break;
			}
		}
	}

	V_ASSERT(result != nullptr);

	if (result == nullptr)
	{
		int* a = 0;
		*a = 0;
	}

	return result;
}

void VNode2D::GetSupportedAnimationChannelsList(std::list<std::string>& pList)
{
	if (!pList.empty())
		pList.clear();

	auto animProps = MetaData()->GetPropertiesByTag("CanBeAnimated");

	for (auto prop : animProps)
		pList.push_back(prop->Name());

	for (auto component : mComponents->GetAll())
	{
		animProps = component->MetaData()->GetPropertiesByTag("CanBeAnimated");

		for (auto prop : animProps)
			pList.push_back(component->GetName() + "." + prop->Name());
	}
}

void RegisterVectorNode2D(VMetaData& pMeta)
{
	pMeta.AddConstructor<std::vector<VNode2D*>()>();
	pMeta.AddConstructor<std::vector<VNode2D*>(const std::vector<VNode2D*>&)>();

	ImplementObjectVector<VNode2D>(pMeta);
}

VTypeInfo<std::vector<VNode2D*>> gMetaVectorNode2D("vector<VNode2D*>", &RegisterVectorNode2D);

void VNode2D::ReflectType(VMetaData& pMeta)
{
	pMeta.AddBaseClass<VObject, VNode2D>();
	pMeta.AddConstructor<VNode2D()>();
	pMeta.AddStaticFunction("RegisterTypeLua", &VNode2D::RegisterTypeLua);
	pMeta.AddProperty("Name", &VNode2D::GetName, &VNode2D::SetName);
	pMeta.AddProperty("UniqueName", &VNode2D::GetUniqueName, &VNode2D::SetUniqueName);
	pMeta.AddProperty("Tag", &VNode2D::GetTag, &VNode2D::SetTag);
	pMeta.AddProperty("PositionPercent", &VNode2D::IsPositionPercent, &VNode2D::SetIsPositionPercent);
	pMeta.AddProperty("AnchorPercent", &VNode2D::IsAnchorPercent, &VNode2D::SetIsAnchorPercent);
	pMeta.AddProperty("SizePercent", &VNode2D::IsSizePercent, &VNode2D::SetIsSizePercent);
	pMeta.AddProperty("Position", &VNode2D::GetPosition, &VNode2D::SetPosition, VAttribute("CanBeAnimated"), VAttribute("Position"));
	pMeta.AddProperty("Rotation", &VNode2D::GetRotation, &VNode2D::SetRotation, VAttribute("CanBeAnimated"), VAttribute("Rotation"));
	pMeta.AddProperty("Scale", &VNode2D::GetScale, &VNode2D::SetScale, VAttribute("CanBeAnimated"), VAttribute("Scale"));
	pMeta.AddProperty("Anchor", &VNode2D::GetAnchor, &VNode2D::SetAnchor, VAttribute("CanBeAnimated"), VAttribute("Anchor"));
	pMeta.AddProperty("Size", &VNode2D::GetSize, &VNode2D::SetSize);
	pMeta.AddProperty("Color", &VNode2D::GetColor, &VNode2D::SetColor, VAttribute("CanBeAnimated"), VAttribute("Color"));
	pMeta.AddProperty("Opacity", &VNode2D::GetOpacity, &VNode2D::SetOpacity, VAttribute("CanBeAnimated"));
	pMeta.AddProperty("Visible", &VNode2D::IsVisible, &VNode2D::SetVisible, VAttribute("CanBeAnimated"));
	pMeta.AddProperty("CascadeOpacity", &VNode2D::IsCascadeOpacityEnabled, &VNode2D::SetCascadeOpacityEnabled);
	pMeta.AddProperty("BlendMode", &VNode2D::GetBlendMode, &VNode2D::SetBlendMode);
	pMeta.AddMethod("FindNode", &VNode2D::FindNode);
	pMeta.AddMethod("GetParent", &VNode2D::GetParent);
	pMeta.AddMethod("SetName", &VNode2D::SetName);
	pMeta.AddMethod("GetChildNode", &VNode2D::GetChildNode);
	pMeta.AddMethod("GetChildsCount", &VNode2D::GetChildsCount);
	pMeta.AddMethod("AttachChild", &VNode2D::AttachChild);
	pMeta.AddMethod("GetChildByTag", static_cast<VNode2D*(VNode2D::*)(int)const>(&VNode2D::GetChildByTag));
	pMeta.AddMethod("AfterDeserialize", &VNode2D::AfterDeserialize, VAttribute("AfterDeserialize"));
	pMeta.AddField("mComponents", &VNode2D::mComponents, VAttribute("Hidden"));
	pMeta.AddField("mChilds", &VNode2D::mChilds, VAttribute("Hidden"));
}

void VNode2D::RegisterTypeLua(VLuaState* pLua)
{
	auto getComponentAuto = [pLua](VNode2D* node, const std::string& name) -> sol::object
	{
		auto result = node->GetComponent(name);

		if (result != nullptr)
		{
			auto metaData = result->MetaData();
			auto value = Any(result).GetValue();
			auto data = value->GetData(metaData);

			return metaData->GetLuaObject(*pLua, data);
		}

		auto scriptComponentMeta = GetMetaData<VComponentScript>();
		auto& components = node->GetAllComponents();

		for (auto component : components)
		{
			if (component->MetaData() == scriptComponentMeta)
			{
				if (auto script = dynamic_cast<VComponentScript*>(component))
				{
					if (script->GetScriptName() == name)
					{
						result = script;

						auto value = Any(result).GetValue();
						auto data = value->GetData(scriptComponentMeta);

						return scriptComponentMeta->GetLuaObject(*pLua, data);
					}
				}
			}
		}

		return sol::lua_nil;
	};

	auto table = pLua->new_usertype<VNode2D>("VNode2D");

	table["Reference"] = &VNode2D::Reference;
	table["Dereference"] = &VNode2D::Dereference;
	table["MetaData"] = &VNode2D::MetaData;
	table["GetOpacity"] = &VNode2D::GetOpacity;
	table["SetOpacity"] = &VNode2D::SetOpacity;
	table["IsVisible"] = &VNode2D::IsVisible;
	table["SetVisible"] = &VNode2D::SetVisible;
	table["GetRotation"] = &VNode2D::GetRotation;
	table["SetRotation"] = &VNode2D::SetRotation;
	table["SetPositionXY"] = &VNode2D::SetPositionXY;
	table["SetPosition"] = &VNode2D::SetPosition;
	table["GetPosition"] = &VNode2D::GetPosition;
	table["GetPositionInPoints"] = &VNode2D::GetPositionInPoints;
	table["GetWorldPosition"] = &VNode2D::GetWorldPosition;
	table["SetScaleXY"] = &VNode2D::SetScaleXY;
	table["SetScale"] = &VNode2D::SetScale;
	table["GetScale"] = &VNode2D::GetScale;
	table["SetSizeXY"] = &VNode2D::SetSizeXY;
	table["SetSize"] = &VNode2D::SetSize;
	table["GetSize"] = &VNode2D::GetSize;
	table["GetSizeInPoints"] = &VNode2D::GetSizeInPoints;
	table["SetIsSizePercent"] = &VNode2D::SetIsSizePercent;
	table["IsPositionPercent"] = &VNode2D::IsPositionPercent;
	table["SetIsPositionPercent"] = &VNode2D::SetIsPositionPercent;
	table["SetAnchorXY"] = &VNode2D::SetAnchorXY;
	table["SetAnchor"] = &VNode2D::SetAnchor;
	table["GetAnchor"] = &VNode2D::GetAnchor;
	table["GetAnchorInPoints"] = &VNode2D::GetAnchorInPoints;
	table["SetColor"] = &VNode2D::SetColor;
	table["GetColor"] = &VNode2D::GetColor;
	table["FindNode"] = &VNode2D::FindNode;
	table["GetChildsCount"] = &VNode2D::GetChildsCount;
	table["GetChildNode"] = &VNode2D::GetChildNode;
	table["GetTag"] = &VNode2D::GetTag;
	table["SetTag"] = &VNode2D::SetTag;
	table["GetIndexOf"] = static_cast<int(VNode2D::*)(VNode2D*)const>(&VNode2D::GetIndexOf);
	table["InsertChild"] = &VNode2D::InsertChild;
	table["GetChildByTag"] = &VNode2D::GetChildByTag;
	table["GetChildsByTag"] = &VNode2D::GetChildsByTag;
	table["AttachChild"] = &VNode2D::AttachChild;
	table["GetComponent"] = &VNode2D::GetComponent;
	table["GetComponentAuto"] = getComponentAuto;
	table["GetComponents"] = &VNode2D::GetComponents;
	table["AddComponent"] = &VNode2D::AddComponent;
	table["GetAllComponents"] = &VNode2D::GetAllComponents;
	table["IsCanAddComponent"] = &VNode2D::IsCanAddComponent;
	table["RemoveComponent"] = sol::overload(
		static_cast<void(VNode2D::*)(const std::string&)>(&VNode2D::RemoveComponent),
		static_cast<void(VNode2D::*)(VComponent*)>(&VNode2D::RemoveComponent)
	);
	table["GetName"] = &VNode2D::GetName;
	table["SetName"] = &VNode2D::SetName;
	table["GetUniqueName"] = &VNode2D::GetUniqueName;
	table["SetUniqueName"] = &VNode2D::SetUniqueName;
	table["GetRoot"] = &VNode2D::GetRoot;
	table["WorldToLocal"] = &VNode2D::WorldToLocal;
	table["LocalToWorld"] = &VNode2D::LocalToWorld;
	table["PickNode"] = &VNode2D::PickNode;
	table["IsSelected"] = &VNode2D::IsSelected;
	table["SetSelected"] = &VNode2D::SetSelected;
	table["IsAnchorPercent"] = &VNode2D::IsAnchorPercent;
	table["SetIsAnchorPercent"] = &VNode2D::SetIsAnchorPercent;
	table["DetachChild"] = &VNode2D::DetachChild;
	table["ShouldUpdateMatrix"] = &VNode2D::ShouldUpdateMatrix;
	table["GetParent"] = &VNode2D::GetParent;
	table["GetParentScene"] = &VNode2D::GetParentScene;
	table["Enumerate"] = &VNode2D::Enumerate;
	table["StartAction"] = &VNode2D::StartAction;
	table["StopActions"] = &VNode2D::StopActions;
	table["StopActionsByTag"] = &VNode2D::StopActionsByTag;
	table["GetActionByTag"] = &VNode2D::GetActionByTag;
	table["GetActionsByTag"] = &VNode2D::GetActionsByTag;
	table["GetRunningActions"] = &VNode2D::GetRunningActions;
	table["GetRunningActionsCount"] = &VNode2D::GetRunningActionsCount;
	table["SetCascadeOpacityEnabled"] = &VNode2D::SetCascadeOpacityEnabled;
	table["IsCascadeOpacityEnabled"] = &VNode2D::IsCascadeOpacityEnabled;
	table["OnPause"] = &VNode2D::OnPause;
	table["OnResume"] = &VNode2D::OnResume;
	table["Update"] = &VNode2D::Update;
	table["IsPaused"] = &VNode2D::IsPaused;
	table["IsStarted"] = &VNode2D::IsStarted;
}

REGISTER_TYPE(VNode2D)

std::string VNode2D::DumpTree(int pLevel) const
{
	std::stringstream stream;
	stream << mName << std::endl;

	pLevel++;

	for (auto child : mChilds)
	{
		for (int i = 0; i < pLevel; i++)
			stream << "| ";

		stream << child->DumpTree(pLevel);
	}

	return stream.str();
}

void VNode2D::SelectIfImpl(std::vector<VNode2D*>& pContainer, const VNode2DSelectorCallback& pSelector)
{
    for (auto child : mChilds)
    {
        if (pSelector(child))
        {
            pContainer.push_back(child);
        }

        child->SelectIfImpl(pContainer, pSelector);
    }
}

void VNode2D::Enumerate(const VNode2DEnumerationCallback& pSelector)
{
	if (pSelector == nullptr)
	{
		return;
	}

	for (auto child : mChilds)
	{
		pSelector(child);
		child->Enumerate(pSelector);
	}
}

std::vector<VNode2D*> VNode2D::SelectIf(const VNode2DSelectorCallback& pSelector)
{
    std::vector<VNode2D*> result;

    if (pSelector == nullptr)
    {
        return result;
    }
    
    if (pSelector(this))
    {
        result.push_back(this);
    }

    SelectIfImpl(result, pSelector);

    return result;
}

VNode2D* VNode2D::FindNode(const std::string& pPath)
{
	if (pPath.size() > 0 && pPath[0] == '@')
	{
		if (!pPath.compare("@" + mUniqueName) && !mUniqueName.empty())
			return this;

		for (auto child : mChilds)
		{
			auto result = child->FindNode(pPath);
			if (result)
				return result;
		}

		return nullptr;
	}

	if (!pPath.compare(mName))
		return this;

	int slash = pPath.find_first_of('/');

	if (slash < 0)
		return 0;

	std::string name = pPath.substr(0, slash);
	std::string path = pPath.substr(slash + 1, pPath.size() - 1);

	if (!mName.compare(name))
	{
		for (auto child : mChilds)
		{
			auto result = child->FindNode(path);
			if (result)
				return result;
		}
	}

	return nullptr;
}

std::string VNode2D::GetPath() const
{
	std::string result = mName;
	VNode2D* parent = mParent;

	while (parent)
	{
		result = parent->mName + "/" + result;
		parent = parent->mParent;
	}

	return result;
}

void VNode2D::SetPositionXY(float pX, float pY)
{
	SetPosition(vec3(pX, pY, mPosition.z));
}

void VNode2D::SetPosition(const vec3& pPos)
{
	if (pPos == mPosition)
	{
		return;
	}

	mPosition = pPos;
	ShouldUpdateMatrix();
}

void VNode2D::SetVisible(bool pVal)
{
	if (pVal != mVisible)
	{
		UpdateCascadeOpacity();
	}

	mVisible = pVal;
}

void VNode2D::SetColor(const VColor& pColor)
{
	mColor = pColor;
	mDisplayedColor.a = pColor.a;
	mNeedUpdateColor = true;
	UpdateCascadeOpacity();
}

void VNode2D::SetRotation(const float& pRot)
{
	if (pRot == mRotation)
	{
		return;
	}

	mRotation = pRot;
	ShouldUpdateMatrix();
}

void VNode2D::SetScaleXY(float pX, float pY)
{
	SetScale(vec2(pX, pY));
}

void VNode2D::SetScale(const vec2& pScale)
{
	if (mScale == pScale)
	{
		return;
	}

	mScale = pScale;
	ShouldUpdateMatrix();
}

void VNode2D::SetAnchorXY(float pX, float pY)
{
	SetAnchor(vec2(pX, pY));
}

void VNode2D::SetAnchor(const vec2& pAnchor)
{
	if (pAnchor == mAnchor)
	{
		return;
	}

	if (mIsAnchorPercent)
		mAnchor.Set(VMath::Clamp(pAnchor.x, 0.0f, 1.0f), VMath::Clamp(pAnchor.y, 0.0f, 1.0f));
	else
		mAnchor = pAnchor;

	ShouldUpdateMatrix();
}

void VNode2D::SetParent(VNode2D* pVal)
{
	mParent = pVal;
	ShouldUpdateMatrix();
}

void VNode2D::SetIsPositionPercent(bool pVal)
{
	if (mIsPositionPercent == pVal)
		return;

	mIsPositionPercent = pVal;
	ShouldUpdateMatrix();
}

void VNode2D::SetIsSizePercent(bool pVal)
{
	if (mIsSizePercent == pVal)
		return;

	mIsSizePercent = pVal;
	ShouldUpdateMatrix();
}

void VNode2D::SetIsAnchorPercent(bool pVal)
{
	if (mIsAnchorPercent == pVal)
		return;

	if (mIsAnchorPercent)
	{
		mAnchor = vec2(mAnchor.x * mSize.x, mAnchor.y * mSize.y);
	}
	else
	{
		mAnchor = vec2(mSize.x != 0.0f ? mAnchor.x / mSize.x : 0.0f,
			mSize.y != 0.0f ? mAnchor.y / mSize.y : 0.0f);
	}

	mIsAnchorPercent = pVal;
	ShouldUpdateMatrix();
}

void VNode2D::SetSizeXY(float pX, float pY)
{
	SetSize(vec2(pX, pY));
}

void VNode2D::SetSize(const vec2& pSize)
{
	if (mSize == pSize)
	{
		return;
	}

	vec2 size = pSize;

	size.x = size.x < 0.0f ? 0.0f : size.x;
	size.y = size.y < 0.0f ? 0.0f : size.y;

	OnResize(&size);

	size.x = size.x < 0.0f ? 0.0f : size.x;
	size.y = size.y < 0.0f ? 0.0f : size.y;

	mSize = size;

	ShouldUpdateMatrix();
}

void VNode2D::SetOpacity(const float& pOpacity)
{
	mColor.a = pOpacity;
	mDisplayedColor.a = pOpacity;
	mNeedUpdateColor = true;
	UpdateCascadeOpacity();
}

bool VNode2D::SetName(const std::string& pName)
{
	if (pName.empty())
		return false;

	if (mParent != 0)
	{
		if (mParent->FindNode(mParent->GetName() + "/" + pName))
		{
			return false;
		}
	}

	mName = pName;
	return true;
}

void VNode2D::OnResize(vec2* pNewSize)
{

}

void VNode2D::SetUniqueName(const std::string& pName)
{
	mUniqueName = pName;
}

float VNode2D::GetRotation()
{
	return mRotation;
}

float VNode2D::GetOpacity()
{
	return mColor.a;
}

vec2 VNode2D::GetScale()
{
	return mScale;
}

vec2 VNode2D::GetSize()
{
	mLastSize = GetSizeInPoints();
	return mSize;
}

vec2 VNode2D::GetAnchor()
{
	return mAnchor;
}

vec3 VNode2D::GetPosition()
{
	return mPosition;
}

const VColor& VNode2D::GetColor()
{
	return mColor;
}

const VColor& VNode2D::GetDisplayedColor()
{
	mDisplayedColor.r = mColor.r;
	mDisplayedColor.g = mColor.g;
	mDisplayedColor.b = mColor.b;
	return mDisplayedColor;
}

float VNode2D::GetDisplayedOpacity()
{
	return mDisplayedColor.a;
}

bool VNode2D::IsCascadeOpacityEnabled() const
{
	return mCascadeOpacityEnabled;
}

void VNode2D::SetCascadeOpacityEnabled(bool pVal)
{
	if (mCascadeOpacityEnabled != pVal)
	{
		mCascadeOpacityEnabled = pVal;

		if (mCascadeOpacityEnabled)
		{
			UpdateCascadeOpacity();
		}
		else
		{
			DisableCascadeOpacity();
		}
	}
}

void VNode2D::UpdateCascadeOpacity()
{
	float parentOpacity = 1.0f;

	if (IsCascadeOpacityEnabled())
	{
		if (mParent != nullptr && mParent->IsCascadeOpacityEnabled())
		{
			parentOpacity = mParent->GetDisplayedOpacity();
		}
	}

	UpdateDisplayedOpacity(parentOpacity);
}

void VNode2D::UpdateDisplayedOpacity(float parentOpacity)
{
	mDisplayedColor.a = mColor.a * parentOpacity;
	OnUpdateColor();

	for (const auto& child : mChilds)
	{
		child->UpdateDisplayedOpacity(mDisplayedColor.a);
	}
}

void VNode2D::DisableCascadeOpacity()
{
	mDisplayedColor.a = mColor.a;
	OnUpdateColor();

	for (const auto& child : mChilds)
	{
		child->UpdateDisplayedOpacity(1.0f);
	}
}

const std::string& VNode2D::GetName() const
{
	return mName;
}

bool VNode2D::IsVisibleRecursive() const
{
	const auto visible = IsVisible();
	return mParent == nullptr ? visible : visible && mParent->IsVisibleRecursive();
}

bool VNode2D::IsVisible() const
{
	return mVisible;
}

bool VNode2D::IsAnchorPercent() const
{
	return mIsAnchorPercent;
}

void VNode2D::SetTag(int pValue)
{
	mTag = std::max(pValue, -1);
}

int VNode2D::GetTag() const
{
	return mTag;
}

VNode2D* VNode2D::GetChildByTag(int pTag) const
{
	const auto size = mChilds.size();

	for (size_t i = 0; i < size; i++)
	{
		auto child = mChilds[i];

		if (child->mTag == pTag)
		{
			return child;
		}
	}

	return nullptr;
}

std::vector<VNode2D*> VNode2D::GetChildsByTag(int pTag) const
{
	std::vector<VNode2D*> result;

	const auto size = mChilds.size();

	for (size_t i = 0; i < size; i++)
	{
		auto child = mChilds[i];

		if (child->mTag == pTag)
		{
			result.push_back(child);
		}
	}

	return result;
}

bool VNode2D::IsPositionPercent() const
{
	return mIsPositionPercent;
}

bool VNode2D::IsSizePercent() const
{
	return mIsSizePercent;
}

const std::string& VNode2D::GetUniqueName() const
{
	return mUniqueName;
}

void VNode2D::OnUpdateMatrix()
{
	const static vec3 zAxis(0.0f, 0.0f, -1.0f);

	const auto anchor = GetAnchorInPoints();
	const auto size = GetSizeInPoints();
	const auto pos = GetPositionInPoints();

	mLastSize = size;
	mWorldMatrix.Identity();

	mWorldMatrix.mtx[0][0] = mScale.x;
	mWorldMatrix.mtx[3][0] = -anchor.x * mScale.x;

	mWorldMatrix.mtx[1][1] = mScale.y;
	mWorldMatrix.mtx[3][1] = anchor.y * mScale.y;

	if (!VMath::CloseEnough(0.0f, mRotation))
	{
		mWorldMatrix.MulRotate(zAxis, mRotation);
	}

	mWorldMatrix.mtx[3][0] += pos.x;
	mWorldMatrix.mtx[3][1] += pos.y;
	mWorldMatrix.mtx[3][2] += pos.z;

	if (mParent != nullptr)
	{
		mWorldMatrix = mWorldMatrix * mParent->mWorldMatrix;
	}
}

void VNode2D::OnUpdateColor()
{
}

void VNode2D::UpdateMatrix()
{
	if (!mNeedUpdateMatrix)
		return;

	if (mParent != 0)
		mParent->UpdateMatrix();

	OnUpdateMatrix();

	mNeedUpdateMatrix = false;
	mNeedUpdateInverseMatrix = true;

	mComponents->OnUpdateMatrix();
}

void VNode2D::UpdateColor()
{
	if (!mNeedUpdateColor)
		return;

	if (mParent != 0)
		mParent->UpdateColor();

	OnUpdateColor();

	mNeedUpdateColor = false;
}

void VNode2D::ShouldUpdateMatrix()
{
	if (mNeedUpdateMatrix)
		return;

	mNeedUpdateMatrix = true;
    
	for (auto child : mChilds)
		child->ShouldUpdateMatrix();
}

void VNode2D::ShouldUpdateColor()
{
	if (mNeedUpdateColor)
		return;

	mNeedUpdateColor = true;

	for (auto child : mChilds)
		child->ShouldUpdateColor();
}

void VNode2D::DeleteChilds()
{
	while (!mChilds.empty())
	{
		DetachChild(mChilds.front());
	}
}

void VNode2D::DetachChilds()
{
	while (!mChilds.empty())
	{
		DetachChild(mChilds.front());
	}
}

bool VNode2D::AttachChild(VNode2D* pChild)
{
	if (pChild == 0)
		return false;
	
	if (!IsCanAttach(pChild->MetaData()))
		return false;

	pChild->Reference();

	if (auto parent = pChild->mParent)
	{
		pChild->mParent->DetachChild(pChild);
	}

	pChild->mParent = this;

	if (mParentScene != nullptr)
	{
		pChild->OnEnterScene(mParentScene);
	}

	pChild->ShouldUpdateMatrix();

	mChilds.push_back(pChild);
	
	if (mIsStarted)
	{
		pChild->OnStart();
	}

	pChild->UpdateCascadeOpacity();
	return true;
}

void VNode2D::DetachChild(VNode2D* pChild)
{
	if (pChild->mParent != this)
		return;

	pChild->mParent = 0;

	if (pChild->mParentScene != nullptr)
	{
		pChild->OnLeaveScene();
	}

	int index = GetIndexOf(pChild);

	if (index >= 0)
		mChilds.erase(mChilds.begin() + index);

	pChild->ShouldUpdateMatrix();
	
	if (mIsStarted)
	{
		pChild->OnStop();
	}

	pChild->Dereference();
}

void VNode2D::OnEnterScene(VScene* pScene)
{
	if (pScene == nullptr)
	{
		V_ASSERT(false);
		return;
	}

	mParentScene = pScene;
	mParentScene->OnNodeEnter(this);

	for (auto child : mChilds)
	{
		child->OnEnterScene(pScene);
	}
}

void VNode2D::OnLeaveScene()
{
	if (mParentScene == nullptr)
	{
		return;
	}

	for (auto child : mChilds)
	{
		child->OnLeaveScene();
	}

	mParentScene->OnNodeLeave(this);
	mParentScene = nullptr;
}

VNode2D* VNode2D::LastChild() const
{
	return mChilds.size() > 0 ? mChilds[0] : 0;
}

bool VNode2D::InsertChild(VNode2D* pChild, int pPos)
{
	if (pPos < 0)
	{
		pPos = 0;
	}
	
	const int size = mChilds.size();

	if (pPos > size)
	{
		pPos = size;
	}

	pChild->Reference();

	if (auto parent = pChild->mParent)
	{
		parent->DetachChild(pChild);
	}
	
	mChilds.insert(mChilds.begin() + pPos, pChild);

	pChild->SetParent(this);
    
	if (mIsStarted)
	{
		pChild->OnStart();
	}

	return true;
}

bool VNode2D::ContainChild(VNode2D* pChild) const
{
	for (size_t i = 0; i < mChilds.size(); i++)
	{
		if (mChilds[i] == pChild)
		{
			return true;
		}
	}

	return false;
}

int VNode2D::GetIndexOf(VNode2D* pChild) const
{
	for (size_t i = 0; i < mChilds.size(); i++)
	{
		if (mChilds[i] == pChild)
		{
			return i;
		}
	}

	return -1;
}

int VNode2D::GetIndexOf(const std::string& pName) const
{
	for (size_t i = 0; i < mChilds.size(); i++)
	{
		if (!mChilds[i]->mName.compare(pName))
		{
			return i;
		}
	}

	return -1;
}

int VNode2D::GetChildsCount() const
{
	return mChilds.size();
}

const std::vector<VNode2D*>& VNode2D::GetChilds() const
{
	return mChilds;
}

VNode2D* VNode2D::GetChildNode(int pIndex)
{
	if (pIndex < 0 || pIndex >= int(mChilds.size()))
		return 0;

	return mChilds[pIndex];
}

VNode2D* VNode2D::GetParent() const
{
	return mParent;
}

VScene* VNode2D::GetParentScene() const
{
	return mParentScene;
}

bool VNode2D::IsChildOfRecursive(const VNode2D* pToCheck) const
{
	if (mParent != nullptr)
	{
		if (mParent == pToCheck)
		{
			return true;
		}
	}
	else
	{
		return false;
	}

	return mParent->IsChildOfRecursive(pToCheck);
}

VNode2D* VNode2D::GetRoot() const
{
	VNode2D* parent = GetParent();

	while (parent && parent->GetParent())
		parent = parent->GetParent();

	return parent;
}

void VNode2D::SetSelected(bool pValue)
{
	mSelected = pValue;
}

bool VNode2D::IsSelected() const
{
	return mSelected;
}

vec3 VNode2D::LocalToWorld(const vec3& pPoint)
{
	UpdateMatrix();

	vec4 result = vec4(pPoint, 1.0f) * mWorldMatrix;
	return vec3(result.x, result.y, result.z);
}

vec3 VNode2D::WorldToLocal(const vec3& pPoint)
{
	UpdateMatrix();

	if (mNeedUpdateInverseMatrix)
	{
		mWorldMatrixInverse = mWorldMatrix.Inverse();
		mNeedUpdateInverseMatrix = false;
	}

	vec4 result = vec4(pPoint, 1.0f) * mWorldMatrixInverse;
	return vec3(result.x, result.y, result.z);
}

float VNode2D::LocalToWorldAngle(const float pAngle)
{
    auto parent = GetParent();
    auto rotation = mRotation;

    while (parent != nullptr)
    {
        rotation += parent->GetRotation();
        parent = parent->GetParent();
    }

    while (rotation >= 360.0f)
    {
        rotation -= 360.0f;
    }

    return rotation;
}

float VNode2D::WorldToLocalAngle(const float pAngle)
{
    V_ASSERT(false);
    return 0.0f;
}

vec3 VNode2D::GetWorldPosition()
{
	const auto anchor = GetAnchorInPoints();	
	return LocalToWorld(vec3(anchor.x, -anchor.y));
}

void VNode2D::SetWorldPosition(const vec3& pPosition)
{
	if (auto parent = GetParent())
	{
		SetPosition(parent->WorldToLocal(pPosition));
	}
	else
	{
		SetPosition(pPosition);
	}
}

vec2 VNode2D::GetWorldScale() const
{
	auto result = mScale;

	if (mParent != nullptr)
	{
		auto parentScale = mParent->GetScale();

		result.x *= parentScale.x;
		result.y *= parentScale.y;
	}

	return result;
}

bool VNode2D::IsCanAttach(const VMetaData* pToAttach) const
{
	return true;
}

VRectF VNode2D::GetLocalRect()
{
	return VRectF(0.0f, GetSizeInPoints());
}

VRectF VNode2D::GetParentSpaceRect()
{
	return VRectF(GetPosition(), GetSizeInPoints());
}

VRectF VNode2D::GetWorldSpaceAABB()
{	
	vec2 size = GetSizeInPoints();

	vec4 vertices[4] = 
	{
		vec4(0.0f, 0.0f, 0.0f, 1.0f) * mWorldMatrix,
		vec4(size.x, 0.0f, 0.0f, 1.0f) * mWorldMatrix,
		vec4(size.x, -size.y, 0.0f, 1.0f) * mWorldMatrix,
		vec4(0.0f, -size.y, 0.0f, 1.0f) * mWorldMatrix
	};
	
	VRectF result(vec2(vertices[0].x, vertices[0].y), vec2(0.0f));

	for (int i = 1; i < sizeof(vertices) / sizeof(vec4); i++)
	{
		result = result.United(VRectF(vec2(vertices[i].x, vertices[i].y), vec2(0.0f)));
	}
	
	return result;
}

VRectF VNode2D::GetParentSpaceAABB()
{	
	vec2 pos = 0;
	vec2 size = GetSizeInPoints();

	vec4 vertices[4] =
	{
		vec4(pos.x, pos.y, 0.0f, 1.0f),
		vec4(pos.x + size.x, pos.y, 0.0f, 1.0f),
		vec4(pos.x + size.x, size.y + pos.y, 0.0f, 1.0f),
		vec4(pos.x, size.y + pos.y, 0.0f, 1.0f)
	};

	VRectF result(vec2(vertices[0].x, vertices[0].y), vec2(0.0f));

	for (int i = 1; i < sizeof(vertices) / sizeof(vec4); i++)
	{
		result = result.United(VRectF(vec2(vertices[i].x, vertices[i].y), vec2(0.0f)));
	}
	
	return result;
}

VRectF VNode2D::GetWorldRect()
{
	return VRectF(LocalToWorld(0.0f), GetSizeInPoints());
}

VNode2D* VNode2D::PickNode(const vec2& pPoint, bool pRecursive)
{
	if (!mVisible)
		return 0;

	if (mNeedUpdateMatrix)
		UpdateMatrix();
	
	if (pRecursive)
	{
		VNode2D* result = 0;

		for (int i = mChilds.size() - 1; i >= 0; i--)
		{
			result = mChilds[i]->PickNode(pPoint);
			if (result)
				return result;
		}
	}

	vec4 vertices[4];
	vec2 size = GetSizeInPoints();

	vertices[0].Set(0.0f, 0.0f, 0.0f, 1.0f);
	vertices[1].Set(size.x, 0.0f, 0.0f, 1.0f);
	vertices[2].Set(size.x, -size.y, 0.0f, 1.0f);
	vertices[3].Set(0.0f, -size.y, 0.0f, 1.0f);

	for (int i = 0; i < sizeof(vertices) / sizeof(vec4); i++)
	{
		vertices[i] = vertices[i] * mWorldMatrix;
	}

	if (VGeometry::IsPointInPolygon(vertices, 4, pPoint) != 0)
		return this;
		
	return 0;
}

VNode2D* VNode2D::PickNode3D(const vec3& pPointStart, const vec3& pPointEnd, std::vector<VNodePickingData>& pResults, bool pRecursive, const int pDepth)
{
	if (!mVisible)
		return nullptr;

	if (mNeedUpdateMatrix)
		UpdateMatrix();

	if (pRecursive)
	{
		for (int i = mChilds.size() - 1; i >= 0; i--)
		{
			mChilds[i]->PickNode3D(pPointStart, pPointEnd, pResults, pRecursive, pDepth + 1);
		}
	}

	auto localStart = WorldToLocal(pPointStart);
	auto localEnd = WorldToLocal(pPointEnd);

	vec4 vertices[4];
	vec2 size = GetSizeInPoints();

	vertices[0].Set(0.0f, 0.0f, 0.0f, 1.0f);
	vertices[1].Set(size.x, 0.0f, 0.0f, 1.0f);
	vertices[2].Set(size.x, -size.y, 0.0f, 1.0f);
	vertices[3].Set(0.0f, -size.y, 0.0f, 1.0f);

	vec3 localPoint;
	
	if (VGeometry::IntersectSegmentPlane(localStart, localEnd, 0.0f, vec3(0.0f, 0.0f, 1.0f), &localPoint))
	{
		if (VGeometry::IsPointInPolygon(vertices, 4, localPoint) != 0)
		{
			pResults.push_back({ this, vec3::Distance(pPointStart, LocalToWorld(localPoint)) });
		}
	}

	if (pDepth == 0 && !pResults.empty())
	{
		stable_sort(pResults.begin(), pResults.end(), [](const VNodePickingData& left, const VNodePickingData& right)
		{
			return left.Distance < right.Distance;
		});

		return pResults[0].Node;
	}

	return nullptr;
}

void VNode2D::DrawSelf()
{
}

AABB2 VNode2D::GetAABB() const
{
	return mAABB;
}

void VNode2D::Update(float pDelta)
{
	if (!mIsPaused)
	{
		UpdateMatrix();
		UpdateColor();

		mComponents->Update(pDelta);
	}

	for (auto child : mChilds)
	{
		child->Update(pDelta);
	}
}

void VNode2D::Draw()
{
	if (!mVisible)
		return;

	UpdateMatrix();
	UpdateColor();

	auto componentsPtr = mComponents.Get();

	componentsPtr->Draw(VComponentDrawEventType_BeforeDrawSelf);

	DrawSelf();

	componentsPtr->Draw(VComponentDrawEventType_AfterDrawSelf);

	for (auto child : mChilds)
		child->Draw();

	componentsPtr->Draw(VComponentDrawEventType_AfterDrawChilds);
}

const mat4x4& VNode2D::GetWorldMatrix() const
{
	return mWorldMatrix;
}

const mat4x4& VNode2D::GetInverseWorldMatrix() const
{
	return mWorldMatrixInverse;
}

std::string VNode2D::GetBlendMode()
{
	if (mUseAutoBlendMode)
	{
		return "Auto";
	}

    switch (mBlendMode)
    {
    case Core::RenderBlendMode::NoBlend:
        return "NoBlend";
    case Core::RenderBlendMode::Normal:
        return "Normal";
    case Core::RenderBlendMode::Additive:
        return "Additive";
    case Core::RenderBlendMode::Multiply:
        return "Multiply";
    case Core::RenderBlendMode::Darken:
        return "Darken";
    case Core::RenderBlendMode::Lighten:
        return "Lighten";
    case Core::RenderBlendMode::Overlay:
        return "Overlay";
	case Core::RenderBlendMode::Premultiplied:
		return "Premultiplied";
    }

    return std::string();
}

void VNode2D::SetBlendMode(const std::string& pMode)
{
	if (pMode == "Auto")
	{
		mUseAutoBlendMode = true;
	}
	else
	{
		if (pMode == "NoBlend")
			mBlendMode = Core::RenderBlendMode::NoBlend;
		else if (pMode == "Normal")
			mBlendMode = Core::RenderBlendMode::Normal;
		else if (pMode == "Additive")
			mBlendMode = Core::RenderBlendMode::Additive;
		else if (pMode == "Multiply")
			mBlendMode = Core::RenderBlendMode::Multiply;
		else if (pMode == "Darken")
			mBlendMode = Core::RenderBlendMode::Darken;
		else if (pMode == "Lighten")
			mBlendMode = Core::RenderBlendMode::Lighten;
		else if (pMode == "Overlay")
			mBlendMode = Core::RenderBlendMode::Overlay;
		else if (pMode == "Premultiplied")
			mBlendMode = Core::RenderBlendMode::Premultiplied;

		mUseAutoBlendMode = false;
	}

	ShouldUpdateColor();
}

bool VNode2D::NeedPremultiplyColor() const
{
	return GetActiveBlendMode() == Core::RenderBlendMode::Premultiplied;
}

Core::RenderBlendMode VNode2D::GetActiveBlendMode() const
{
	if (mUseAutoBlendMode)
	{
		return Core::RenderBlendMode::Normal;
	}
	
	return mBlendMode;
}

void VNode2D::AfterDeserialize()
{
	for (auto node : mChilds)
	{
		node->SetParent(this);
		node->ShouldUpdateMatrix();
		node->ShouldUpdateColor();
		node->UpdateCascadeOpacity();
	}

    mComponents->Init(this);
}

void VNode2D::DrawDebug()
{
	if (mNeedUpdateMatrix)
		UpdateMatrix();

	mat4x4 oldWorldMtx = gRender->GetMatrix(Core::RenderMatrixType::Model);

	gRender->SetBlendMode(Core::RenderBlendMode::Normal);
	gRender->SetShader(gRender->mDefaultShaderColored);		
	gRender->SetMatrix(Core::RenderMatrixType::Model, mWorldMatrix);

	VVertex vertices[4];

	auto size = GetSizeInPoints();
	auto anchor = GetAnchorInPoints();

	vertices[0].SetXY(0.0f, 0.0f);
	vertices[1].SetXY(size.x, 0.0f);
	vertices[2].SetXY(size.x, -size.y);
	vertices[3].SetXY(0.0f, -size.y);

	vertices[0].Color = VColor::Green;
	vertices[1].Color = VColor::Green;
	vertices[2].Color = VColor::Green;
	vertices[3].Color = VColor::Green;

	uint16_t indices[] = 
	{
		0, 1,
		1, 2,
		2, 3,
		3, 0
	};
	
	gRender->DrawElements(Core::RenderPrimitiveType::Lines, vertices, ArraySize(vertices), indices, ArraySize(indices));

	gRender->DrawLine(vec2(anchor.x - 1, -anchor.y), vec2(anchor.x + 1, -anchor.y), VColor::Red);
	gRender->DrawLine(vec2(anchor.x, -anchor.y - 1), vec2(anchor.x, -anchor.y + 1), VColor::Red);

	gRender->SetMatrix(Core::RenderMatrixType::Model, oldWorldMtx);
}

void VNode2D::SetActionHandle(const ptr<VActionHandle>& pHandle)
{
#ifdef _DEBUG
	if (mActionHandle && mActionHandle != pHandle)
	{
		V_ASSERT(mActionHandle->IsEmpty());
	}
#endif

	mActionHandle = pHandle;
}

void VNode2D::StartAction(VAction* pAction)
{
	VActionManager::Instance()->RunAction(this, pAction);
}

void VNode2D::StopActions()
{
	if (mActionHandle)
	{
		mActionHandle->StopAll();
	}
}

void VNode2D::StopActionsByTag(int pTag)
{
	if (mActionHandle)
	{
		mActionHandle->StopAllByTag(pTag);
	}
}

VAction* VNode2D::GetActionByTag(int pTag) const
{
	if (mActionHandle)
	{
		return mActionHandle->GetActionByTag(pTag);
	}

	return nullptr;
}

std::vector<VAction*> VNode2D::GetActionsByTag(int pTag) const
{
	if (mActionHandle)
	{
		return mActionHandle->GetActionsByTag(pTag);
	}

	return std::vector<VAction*>();
}

const std::vector<VAction*>& VNode2D::GetRunningActions() const
{
	const static std::vector<VAction*> sEmpty;

	if (mActionHandle)
	{
		return mActionHandle->GetActions();
	}

	return sEmpty;
}

size_t VNode2D::GetRunningActionsCount() const
{
	return GetRunningActions().size();
}

vec3 VNode2D::GetPositionInPoints() const
{
	if (mIsPositionPercent)
	{
		if (mParent)
		{
			const auto parentSize = mParent->GetSizeInPoints();
			return vec3(mPosition.x * parentSize.x, mPosition.y * parentSize.y, mPosition.z);
		}
		else
		{
			return vec3(0.0f, 0.0f, mPosition.z);
		}
	}
	else
	{
		return mPosition;
	}
}

vec2 VNode2D::GetAnchorInPoints() const
{
	if (mIsAnchorPercent)
	{
		const auto size = GetSizeInPoints();
		return vec2(mAnchor.x * size.x, mAnchor.y * size.y);
	}
	else
	{
		return mAnchor;
	}
}

vec2 VNode2D::GetSizeInPoints() const
{
	if (mIsSizePercent)
	{
		if (mParent)
		{
			const auto parentSize = mParent->GetSizeInPoints();
			return vec2(mSize.x * parentSize.x, mSize.y * parentSize.y);
		}
		else
		{
			return vec2::Zero;
		}
	}
	else
	{
		return mSize;
	}
}