#pragma once

#include <Input/Mouse.h>
#include <Input/Keyboard.h>
#include <Reflection/Object.h>
#include <Reflection/ScriptingEngine.h>

enum VFadeDirection
{
    VFadeDirection_FadeUp,
    VFadeDirection_FadeDown
};

class VSceneManager;
class VSceneView;
class VUICanvas;
class VNode2D;

class VScene : public VObject
{
	MANAGED_TYPE(VScene)

private:
	std::string mName;

	ptr<VNode2D> mRootNode = nullptr;
	VSceneView* mSceneView = nullptr;
	std::vector<VUICanvas*> mUI;

protected:
	bool mDrawOrigin = false;
	bool mEditorMode = false;
	bool mPaused = false;
	bool mPausedUI = false;
	bool mLoaded = false;

public:
	VScene();
	VScene(const std::string& pName);

	bool IsLoaded() const;
	std::string GetName() const;

	virtual ~VScene();
	
	void SetView(VSceneView* pView);
	VSceneView* GetView() const;

	bool IsPaused() const;
	bool IsPausedUI() const;

	virtual void Resume();
	virtual void Pause(bool pPauseUI);

	virtual bool Load();
	virtual void Unload();

	virtual void OnBecomeActive();
	virtual void OnBecomeInactive();

	virtual void Update(float pDt);
	virtual void Draw();

	virtual void OnWindowResize(int pWidth, int pHeight);
	virtual void OnMouseEvent(VMouseEvent* pEvent);
	virtual void OnKeyboardEvent(VKeyboardEvent* pEvent);

	virtual void OnNodeEnter(VNode2D* pNode);
	virtual void OnNodeLeave(VNode2D* pNode);
	virtual void OnUIOrderChanged();

	bool IsEditorMode() const;

	VNode2D* GetRoot() const;
	void SetRoot(const ptr<VNode2D>& pRoot);

	void DrawRoot();
	void DrawGrid();
	void DrawUI();

	static void ReflectType(VMetaData& pMeta);
	static void RegisterTypeLua(VLuaState* pLua);
};

class VSceneManager
{	
private:
	ptr<VScene> mCurrentScene;
	ptr<VScene> mNextScene;

    std::list<ptr<VScene>> mScenes;

    float mFadeTime;
    float mFadeCoeff;

    bool mIsFadeNeeded;

    VFadeDirection mFadeDirection;

public:
	VSceneManager();
	~VSceneManager();

    bool Load();
    void Unload();
	
	ptr<VScene> GetCurrentScene() const;

	void AddScene(ptr<VScene> pScene);
    void RemoveScene(const std::string& pName);

    void SetFadeTime(float pTime);
	ptr<VScene> FindScene(const std::string& pName);

    void SetCurrentScene(const std::string& pName);
    void ChangeScene(const std::string& pName, bool pUseFade);

    void Update(float pDt);
    void Draw();

	void OnWindowResize(int pWidth, int pHeight);
	void OnMouseEvent(VMouseEvent* pEvent);
    void OnKeyboardEvent(VKeyboardEvent* pEvent);

	static VSceneManager* Instance();

private:
    void OnFadeUp();
    void OnFadeDown();
    void FadeTo(VFadeDirection pDirection);
};
