#pragma once

#include <Utils/Utils.h>
#include <Reflection/ScriptingEngine.h>
#include <Render/RenderInterface.h>
#include <Components/Component.h>
#include <Actions/ActionHandle.h>

class VNodeChannelConnectorBase;
class VAnimationChannelBase;
class VScene;

class VNode2D : public VObject
{
	MANAGED_TYPE_BASE(VNode2D)

protected:
	bool mNeedUpdateMatrix;
	bool mNeedUpdateInverseMatrix;
	bool mNeedUpdateColor;

	VScene* mParentScene;
	VNode2D* mParent;
	std::vector<VNode2D*> mChilds;
			
	bool mIsAnchorPercent;
	bool mIsPositionPercent;
	bool mIsSizePercent;
	bool mVisible;
	bool mSelected;
	bool mIsPaused;
	bool mIsStarted;
	bool mCullingEnabled;
	bool mCascadeOpacityEnabled;

	std::string mName;
	std::string mUniqueName;
	vec3 mPosition;
	VColor mColor;
	VColor mDisplayedColor;
	float mRotation;
	vec2 mScale;
	vec2 mSize;
	vec2 mAnchor;

	int mTag;
	AABB2 mAABB;

	vec2 mLastSize;
	mat4x4 mWorldMatrix;
	mutable mat4x4 mWorldMatrixInverse;

	bool mUseAutoBlendMode;
	Core::RenderBlendMode mBlendMode;

	ptr<VComponentSet> mComponents;
	ptr<VActionHandle> mActionHandle;

protected:
	void DisableCascadeOpacity();
	void UpdateDisplayedOpacity(float parentOpacity);

public:
    typedef std::function<bool(VNode2D*)> VNode2DSelectorCallback;
	typedef std::function<void(VNode2D*)> VNode2DEnumerationCallback;
	
    const std::vector<VComponent*>& GetAllComponents();
	VComponent* GetComponent(const std::string& pName);
	std::vector<VComponent*> GetComponents(const std::string& pName);
    bool IsCanAddComponent(const std::string& pComponentName) const;
	virtual void AddComponent(VComponent* pComponent);
	virtual void RemoveComponent(VComponent* pComponent);
	virtual void RemoveComponent(const std::string& pName);
	virtual void RemoveAllComponents();

	VNode2D();

	virtual ~VNode2D();
	
	const mat4x4& GetWorldMatrix() const;
	const mat4x4& GetInverseWorldMatrix() const;

    std::string GetBlendMode();
    void SetBlendMode(const std::string& pMode);

	bool NeedPremultiplyColor() const;
	virtual Core::RenderBlendMode GetActiveBlendMode() const;

	void Cleanup();
	
	virtual VNodeChannelConnectorBase* NewChannelConnector(VAnimationChannelBase* pChannel);
	virtual void GetSupportedAnimationChannelsList(std::list<std::string>& pList);
	
	void SetPositionXY(float pX, float pY);
	void SetAnchorXY(float pX, float pY);
	void SetSizeXY(float pX, float pY);
	void SetScaleXY(float pX, float pY);

	virtual void SetPosition(const vec3& pPos);
	virtual void SetAnchor(const vec2& pAnchor);
	virtual void SetColor(const VColor& pColor);
	virtual void SetRotation(const float& pRot);
	virtual void SetScale(const vec2& pScale);
	virtual void SetSize(const vec2& pSize);
	virtual void SetOpacity(const float& pOpacity);
	virtual void SetVisible(bool pVal);
	virtual void OnResize(vec2* pNewSize);

	bool SetName(const std::string& pName);
	void SetUniqueName(const std::string& pName);	
	void SetParent(VNode2D* pVal);
	void SetTag(int pValue);
		
	virtual float GetRotation();
	virtual float GetOpacity();
	virtual vec2 GetScale();
	virtual vec2 GetSize();
	virtual vec2 GetAnchor();
	virtual vec3 GetPosition();
	virtual const VColor& GetColor();
	virtual const VColor& GetDisplayedColor();
	virtual float GetDisplayedOpacity();

	int GetTag() const;

	const std::string& GetName() const;
	const std::string& GetUniqueName() const;

	virtual bool IsVisible() const;
	virtual bool IsAnchorPercent() const;
	virtual bool IsPositionPercent() const;
	virtual bool IsSizePercent() const;
	virtual bool IsCascadeOpacityEnabled() const;

	bool IsVisibleRecursive() const;

	virtual void SetIsAnchorPercent(bool pVal);
	virtual void SetIsPositionPercent(bool pVal);
	virtual void SetIsSizePercent(bool pVal);
	virtual void SetCascadeOpacityEnabled(bool pVal);

	virtual void OnUpdateMatrix();
	virtual void OnUpdateColor();

	bool IsCullingEnabled() const;
	void SetCullingEnabled(bool pValue);

	bool IsStarted() const;	
	bool IsPaused() const;

	virtual void OnStart();
	virtual void OnStop();

	virtual void OnResume();
	virtual void OnPause();

	void ShouldUpdateMatrix();
	void ShouldUpdateColor();

	void UpdateMatrix();
	void UpdateColor();
	void UpdateCascadeOpacity();
	
	virtual bool AttachChild(VNode2D* pChild);
	virtual void DetachChild(VNode2D* pChild);
	
	virtual void OnEnterScene(VScene* pScene);
	virtual void OnLeaveScene();

	VScene* GetParentScene() const;
	VNode2D* GetParent() const;
	VNode2D* GetRoot() const;
	VNode2D* GetChildByTag(int pTag) const;
	std::vector<VNode2D*> GetChildsByTag(int pTag) const;
	bool IsChildOfRecursive(const VNode2D* pToCheck) const;
	virtual VNode2D* GetChildNode(int pIndex);
	virtual VNode2D* LastChild() const;
	virtual bool InsertChild(VNode2D* pChild, int pPos);
	virtual bool ContainChild(VNode2D* pChild) const;
	virtual int GetIndexOf(VNode2D* pChild) const;
	virtual int GetIndexOf(const std::string& pName) const;
	int GetChildsCount() const;

	const std::vector<VNode2D*>& GetChilds() const;

	void DeleteChilds();
	void DetachChilds();

	VRectF GetLocalRect();
	VRectF GetWorldRect();
	VRectF GetParentSpaceRect();

	VRectF GetWorldSpaceAABB();
	VRectF GetParentSpaceAABB();

	AABB2 GetAABB() const;

	vec3 LocalToWorld(const vec3& pPoint);
	vec3 WorldToLocal(const vec3& pPoint);

    float LocalToWorldAngle(const float pAngle);
    float WorldToLocalAngle(const float pAngle);

	vec3 GetWorldPosition();
	void SetWorldPosition(const vec3& pPosition);

	vec2 GetWorldScale() const;

	virtual bool IsCanAttach(const VMetaData* pToAttach) const;

	virtual VNode2D* PickNode(const vec2& pPoint, bool pRecursive = true);

	virtual VNode2D* PickNode3D(const vec3& pPointStart, const vec3& pPointEnd,
		std::vector<VNodePickingData>& pResults,
		bool pRecursive = true, 
		const int pDepth = 0);
	
	void SetSelected(bool pValue);
	bool IsSelected() const;
	
	void SetActionHandle(const ptr<VActionHandle>& pHandle);
	void StartAction(VAction* pAction);
	void StopActions();
	void StopActionsByTag(int pTag);
	VAction* GetActionByTag(int pTag) const;
	std::vector<VAction*> GetActionsByTag(int pTag) const;
	const std::vector<VAction*>& GetRunningActions() const;
	size_t GetRunningActionsCount() const;

	std::string GetPath() const;
	VNode2D* FindNode(const std::string& pPath);

	std::string DumpTree(int pLevel = 0) const;

    std::vector<VNode2D*> SelectIf(const VNode2DSelectorCallback& pSelector);
	void Enumerate(const VNode2DEnumerationCallback& pSelector);

	virtual void Draw();
	virtual void Update(float pDelta);

	virtual void DrawDebug();
    virtual void AfterDeserialize();

	virtual vec3 GetPositionInPoints() const;
	virtual vec2 GetAnchorInPoints() const;
	virtual vec2 GetSizeInPoints() const;

	static void ReflectType(VMetaData& pMeta);
	static void RegisterTypeLua(VLuaState* pLua);

	virtual void DrawSelf();

private:
    void SelectIfImpl(std::vector<VNode2D*>& pContainer, const VNode2DSelectorCallback& pSelector);
};
