#include "Scene/Scene.h"
#include "Render/RenderInterface.h"
#include "Reflection/FunctionWrap.h"
#include "Nodes/Node2D.h"
#include "UI/UICanvas.h"
#include "UI/GLWidget.h"

void VScene::ReflectType(VMetaData& pMeta)
{
	pMeta.AddBaseClass<VObject, VScene>();
	pMeta.AddConstructor<VScene()>();
	pMeta.AddStaticFunction("RegisterTypeLua", &VScene::RegisterTypeLua);
}

void VScene::RegisterTypeLua(VLuaState* pLua)
{
	pLua->new_usertype<VScene>("VScene",
		"IsEditorMode", &VScene::IsEditorMode,
		"IsPaused", &VScene::IsPaused,
		"IsPausedUI", &VScene::IsPausedUI,
		"GetRoot", &VScene::GetRoot,
		"GetView", &VScene::GetView);
}

REGISTER_TYPE(VScene)

VScene::VScene()
{
	V_ASSERT(false);
}

VScene::VScene(const std::string& pName)
{
	mLoaded = false;
	mName = pName;
	SetRoot(nullptr);
}

VNode2D* VScene::GetRoot() const
{
	return mRootNode.Get();
}

void VScene::SetRoot(const ptr<VNode2D>& pRoot)
{
	if (mRootNode != nullptr)
	{
		mRootNode->OnLeaveScene();
	}

	mRootNode = pRoot;

	if (mRootNode == nullptr)
	{
		mRootNode = new VNode2D();
		mRootNode->SetName("root");
	}

	mRootNode->OnEnterScene(this);
}

bool VScene::IsLoaded() const 
{ 
	return mLoaded;
}

std::string VScene::GetName() const 
{ 
	return mName; 
}

void VScene::SetView(VSceneView* pView)
{
	mSceneView = pView;
}

VSceneView* VScene::GetView() const
{
	return mSceneView;
}

bool VScene::IsPaused() const
{
	return mPaused;
}

bool VScene::IsPausedUI() const
{
	return mPausedUI;
}

void VScene::Resume()
{
	if (!mPaused)
	{
		return;
	}

	const auto uiMetaData = GetMetaData<VUICanvas>();

	mRootNode->Enumerate([&](VNode2D* node)
	{
		if (!mPausedUI)
		{
			if (node->MetaData() == uiMetaData)
			{
				return;
			}

			auto parent = node->GetParent();

			while (parent)
			{
				if (parent->MetaData() == uiMetaData)
				{
					return;
				}

				parent = parent->GetParent();
			}
		}

		if (node->IsPaused())
		{
			node->OnResume();
		}
	});

	if (mRootNode->IsPaused())
	{
		mRootNode->OnResume();
	}

	mPaused = false;
	mPausedUI = false;
}

void VScene::Pause(bool pPauseUI)
{
	if (mPaused)
	{
		return;
	}

	mPaused = true;
	mPausedUI = pPauseUI;

	const auto uiMetaData = GetMetaData<VUICanvas>();

	mRootNode->Enumerate([&](VNode2D* node)
	{
		if (!mPausedUI)
		{
			if (node->MetaData() == uiMetaData)
			{
				return;
			}

			auto parent = node->GetParent();

			while (parent)
			{
				if (parent->MetaData() == uiMetaData)
				{
					return;
				}

				parent = parent->GetParent();
			}
		}

		if (!node->IsPaused())
		{
			node->OnPause();
		}
	});

	if (!mRootNode->IsPaused())
	{
		mRootNode->OnPause();
	}
}

VScene::~VScene() 
{ 
}

bool VScene::Load() 
{
	return true; 
}

void VScene::Unload()
{
	SetRoot(nullptr);
}

void VScene::OnBecomeActive() 
{ 
}

void VScene::OnBecomeInactive() 
{
}

void VScene::Update(float pDt)
{
}

void VScene::Draw()
{
}

void VScene::OnWindowResize(int pWidth, int pHeight) 
{ 
}

void VScene::OnMouseEvent(VMouseEvent* pEvent) 
{ 
}

void VScene::OnKeyboardEvent(VKeyboardEvent* pEvent) 
{ 
}

void VScene::OnNodeEnter(VNode2D* pNode)
{
	if (pNode->MetaData() == GetMetaData<VUICanvas>())
	{
		auto canvas = dynamic_cast<VUICanvas*>(pNode);
		mUI.push_back(canvas);
		OnUIOrderChanged();
		canvas->UpdateBounds();
	}
}

void VScene::OnNodeLeave(VNode2D* pNode)
{
	if (pNode->MetaData() == GetMetaData<VUICanvas>())
	{
		const auto result = std::find(mUI.begin(), mUI.end(), pNode);

		if (result != mUI.end())
		{
			mUI.erase(result);
		}
	}
}

void VScene::OnUIOrderChanged()
{
	if (mUI.empty())
	{
		return;
	}

	std::stable_sort(mUI.begin(), mUI.end(), [](VUICanvas* left, VUICanvas* right)
	{
		return left->GetCanvasZOrder() < right->GetCanvasZOrder();
	});
}

bool VScene::IsEditorMode() const
{
	return mEditorMode;
}

void VScene::DrawRoot()
{
	gRender->Push(Core::RenderState::ModelMatrix);
	gRender->SetDepthTestEnabled(true);
	gRender->SetDepthWriteEnabled(true);
	gRender->SetDepthFunc(Core::RenderCompareFunc::LessEqual);

	if (mDrawOrigin && mEditorMode)
	{
		DrawGrid();
		gRender->Flush();
	}

	//gRender->SetDepthWriteEnabled(false);

	mRootNode->Draw();

	gRender->Flush();
	gRender->Pop(Core::RenderState::ModelMatrix);
}

void VScene::DrawUI()
{
	if (mUI.empty())
	{
		return;
	}

	gRender->SetDepthTestEnabled(false);
	gRender->SetDepthWriteEnabled(false);

	for (auto canvas : mUI)
	{
		if (canvas->IsVisibleRecursive())
		{
			canvas->DrawUI();
		}
	}
}

void VScene::DrawGrid()
{
	gRender->DrawLine(vec3(-1000, 0, 0), vec3(1000, 0, 0), VColor(0, 0, 0, 0.4));
	gRender->DrawLine(vec3(0, -1000, 0), vec3(0, 1000, 0), VColor(0, 0, 0, 0.4));
	gRender->DrawLine(vec3(0, 0, -1000), vec3(0, 0, 1000), VColor(0, 0, 0, 0.4));
}

bool VSceneManager::Load()
{
    return true;
}

void VSceneManager::Unload()
{
    for (auto i = mScenes.begin(); i != mScenes.end(); i++)
    {
		if (auto scene = *i)
		{
			if (scene->IsLoaded())
			{
				scene->Unload();
			}
		}
    }

	mScenes.clear();

	mCurrentScene = nullptr;
	mNextScene = nullptr;
}

void VSceneManager::OnFadeUp()
{
	if (mCurrentScene)
	{
		mCurrentScene->OnBecomeInactive();
		mCurrentScene->Unload();
	}

    mCurrentScene = mNextScene;
	mNextScene = nullptr;

    if(!mCurrentScene->IsLoaded())
        mCurrentScene->Load();

    mCurrentScene->OnBecomeActive();
    FadeTo(VFadeDirection_FadeDown);
}

void VSceneManager::SetFadeTime(float pTime)
{
    mFadeTime = pTime;
}

void VSceneManager::OnFadeDown()
{
    mIsFadeNeeded = false;
}

void VSceneManager::Draw()
{
	if(mCurrentScene)
		mCurrentScene->Draw();

	return;

    if(mIsFadeNeeded)
    {
		gRender->Push(Core::RenderState::ModelMatrix);
		gRender->Push(Core::RenderState::ViewMatrix);
		gRender->Push(Core::RenderState::ProjectionMatrix);

		gRender->SetIdentity(Core::RenderMatrixType::Model);
		gRender->SetIdentity(Core::RenderMatrixType::View);
		gRender->SetMatrix(Core::RenderMatrixType::Projection, mat4x4::CreateOrtho(0.0f, float(gRender->GetViewportWidth()), 0.0f, float(gRender->GetViewportHeight()), -1.0f, 1.0f));

		gRender->FillRect2D(vec2(0.0f), vec2(float(gRender->GetViewportWidth()), float(gRender->GetViewportHeight())), VColor(1.0f, 1.0f, 1.0f, mFadeCoeff / mFadeTime));

		gRender->Pop(Core::RenderState::ModelMatrix);
		gRender->Pop(Core::RenderState::ViewMatrix);
		gRender->Pop(Core::RenderState::ProjectionMatrix);
    }
}

void VSceneManager::FadeTo(VFadeDirection pDirection)
{
    mFadeDirection = pDirection;

    switch(mFadeDirection)
    {
        case VFadeDirection_FadeUp:
            {
                mFadeCoeff = 0.0f;
                mIsFadeNeeded = true;
            }
        break;

        case VFadeDirection_FadeDown:
            {
                mFadeCoeff = mFadeTime;
                mIsFadeNeeded = true;
            }
        break;
    }
}

ptr<VScene> VSceneManager::FindScene(const std::string &pName)
{
	for (auto i = mScenes.begin(), end = mScenes.end(); i != end; i++)
	{
		if (auto scene = *i)
		{
			if (scene->GetName() == pName)
				return scene;
		}
	}

	return nullptr;
}

ptr<VScene> VSceneManager::GetCurrentScene() const
{
	return mCurrentScene;
}

void VSceneManager::AddScene(ptr<VScene> pScene)
{
    if(pScene)
    {
		if (auto iterator = FindScene(pScene->GetName()))
			return;
		else
			mScenes.push_back(pScene);
    }
}

void VSceneManager::RemoveScene(const std::string& pName)
{
	for (auto it = mScenes.begin(); it != mScenes.end(); it++)
	{
		if ((*it)->GetName() == pName)
		{
			auto scene = *it;

			if (mCurrentScene == scene)
				mCurrentScene = nullptr;

			if (scene->IsLoaded())
				scene->Unload();

			mScenes.erase(it);

			break;
		}
	}
}

void VSceneManager::SetCurrentScene(const std::string& pName)
{
	auto scene = FindScene(pName);

	if (scene == mCurrentScene)
	{
		LOGI("Unable to set scene '%s' as current, because it is current already.", pName.c_str());
		return;
	}

	if (scene)
    {
		if (mCurrentScene)
		{
			mCurrentScene->OnBecomeInactive();

			if (mCurrentScene->IsLoaded())
				mCurrentScene->Unload();
		}
		
		mCurrentScene = scene;

		if (!mCurrentScene->IsLoaded())
			mCurrentScene->Load();

		mCurrentScene->OnBecomeActive();
    }
}

void VSceneManager::Update(float pDt)
{
    if(mIsFadeNeeded)
    {
        switch(mFadeDirection)
        {
        case VFadeDirection_FadeUp:
			mFadeCoeff += pDt;

            if(mFadeCoeff > mFadeTime)
                OnFadeUp();
            break;

        case VFadeDirection_FadeDown:
            mFadeCoeff -= pDt;

            if(mFadeCoeff <= 0.0f)
                OnFadeDown();
            break;
        }
    }

    if(mCurrentScene)
        mCurrentScene->Update(pDt);
}

void VSceneManager::ChangeScene(const std::string& pName, bool pUseFade)
{
	auto scene = FindScene(pName);

	if (scene == mCurrentScene)
	{
		LOGI("Unable to set scene '%s' as current, because it is current already.", pName.c_str());
		return;
	}

	if (scene)
	{
        if(!pUseFade || mCurrentScene)
        {
            mCurrentScene = scene;

			auto loaded = mCurrentScene->IsLoaded();

			if (!loaded)
			{
				loaded = mCurrentScene->Load();
			}

			if (loaded)
			{
				mCurrentScene->OnBecomeActive();
			}

            FadeTo(VFadeDirection_FadeDown);
        }
        else
        {
            mNextScene = scene;
            FadeTo(VFadeDirection_FadeUp);
        }
    }
}

VSceneManager::VSceneManager() 
{
	mCurrentScene = nullptr;
	mNextScene = nullptr;

	mFadeTime = 1.0f;
	mFadeCoeff = 1.0f;

	mIsFadeNeeded = false;

	mFadeDirection = VFadeDirection_FadeUp;
}

VSceneManager::~VSceneManager()
{
}

void VSceneManager::OnWindowResize(int pWidth, int pHeight)
{
	if (mCurrentScene)
		mCurrentScene->OnWindowResize(pWidth, pHeight);
}

void VSceneManager::OnMouseEvent(VMouseEvent* pEvent)
{
	if (mCurrentScene)
		mCurrentScene->OnMouseEvent(pEvent);
}

void VSceneManager::OnKeyboardEvent(VKeyboardEvent* pEvent)
{
    if (mCurrentScene)
        mCurrentScene->OnKeyboardEvent(pEvent);
}

VSceneManager* VSceneManager::Instance()
{
	static VSceneManager theInstance;
	return &theInstance;
}