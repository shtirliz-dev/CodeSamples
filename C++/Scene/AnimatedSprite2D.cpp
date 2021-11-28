
#include "AnimatedSprite2D.h"
#include "Reflection/FunctionWrap.h"

void VAnimatedSprite2D::ReflectType(VMetaData& pMeta)
{
	pMeta.AddBaseClass<VSprite2D, VAnimatedSprite2D>();
	pMeta.AddConstructor<VAnimatedSprite2D()>();
	pMeta.AddStaticFunction("RegisterTypeLua", &VAnimatedSprite2D::RegisterTypeLua);
	pMeta.AddProperty("FrameSize", &VAnimatedSprite2D::GetFrameSize, &VAnimatedSprite2D::SetFrameSize);
	pMeta.AddProperty("FrameCount", &VAnimatedSprite2D::GetFrameCount, &VAnimatedSprite2D::SetFrameCount);
	pMeta.AddProperty("FPS", &VAnimatedSprite2D::GetFPS, &VAnimatedSprite2D::SetFPS);
	pMeta.AddProperty("Loop", &VAnimatedSprite2D::IsLoop, &VAnimatedSprite2D::SetLoop);
	pMeta.AddProperty("Active", &VAnimatedSprite2D::IsActive, &VAnimatedSprite2D::SetActive);
}

void VAnimatedSprite2D::RegisterTypeLua(VLuaState* pLua)
{
	pLua->new_usertype<VAnimatedSprite2D>("VAnimatedSprite2D", sol::base_classes, sol::bases<VNode2D, VSprite2D>(),
		"GetFrameSize", &VAnimatedSprite2D::GetFrameSize,
		"SetFrameSize", &VAnimatedSprite2D::SetFrameSize,
		"GetRealFrameCount", &VAnimatedSprite2D::GetRealFrameCount,
		"GetFrameCount", &VAnimatedSprite2D::GetFrameCount,
		"SetFrameCount", &VAnimatedSprite2D::SetFrameCount,
		"GetFPS", &VAnimatedSprite2D::GetFPS,
		"SetFPS", &VAnimatedSprite2D::SetFPS,
		"IsLoop", &VAnimatedSprite2D::IsLoop,
		"SetLoop", &VAnimatedSprite2D::SetLoop,
		"Play", &VAnimatedSprite2D::Play,
		"Pause", &VAnimatedSprite2D::Pause,
		"Stop", &VAnimatedSprite2D::Stop,
		"GetDuration", &VAnimatedSprite2D::GetDuration,
		"Rewind", &VAnimatedSprite2D::Rewind,
		"IsActive", &VAnimatedSprite2D::IsActive);
}

REGISTER_TYPE(VAnimatedSprite2D)

VAnimatedSprite2D::VAnimatedSprite2D()
{
}

VNode2D* VAnimatedSprite2D::PickNode(const vec2& pPoint, bool pRecursive)
{
	return VNode2D::PickNode(pPoint, pRecursive);
}

VNode2D* VAnimatedSprite2D::PickNode3D(const vec3& pPointStart, const vec3& pPointEnd, std::vector<VNodePickingData>& pResults, bool pRecursive,	const int pDepth)
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
	vec2 size = GetSize();

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

void VAnimatedSprite2D::UpdateFrameCache()
{
	if (!mCacheDirty)
	{
		return;
	}

	mFrameCache.clear();

	if (!mTexture || mTexture->IsLoadingFailed())
	{
		mCacheDirty = false;
		return;
	}
	
	mCacheDirty = false;

	const vec2 textureSize(mTexture->Width(), mTexture->Height());
	const vec2 frameUVSize(mFrameSize.x / textureSize.x, mFrameSize.y / textureSize.y);
	const auto maxColFrames = int(floorf(textureSize.x / mFrameSize.x));
	const auto maxRowFrames = int(floorf(textureSize.y / mFrameSize.y));

	mRealFrameCount = std::min(maxRowFrames * maxColFrames, mFrameCount);
	
	mFrameCache.resize(mRealFrameCount);

	if (mRealFrameCount > 0)
	{
		mCurrentFrame = std::min(mCurrentFrame, mRealFrameCount - 1);
	}
	else
	{
		mCurrentFrame = 0;
	}

	auto counter = 0;

	for (auto row = 0; row < maxRowFrames && counter < mRealFrameCount; row++)
	{
		for (auto col = 0; col < maxColFrames && counter < mRealFrameCount; col++)
		{
			auto& uv = mFrameCache[counter++];

			uv.SetPosition(vec2(frameUVSize.x * col, 1.0f - frameUVSize.y * (row + 1)));
			uv.SetSize(vec2(uv.Position().x + frameUVSize.x, uv.Position().y + frameUVSize.y));
		}
	}

	if (mFPS > 0)
	{
		mMaxTime = float(mRealFrameCount) / float(mFPS);
	}
	else
	{
		mCurrentTime = 0.0f;
		mCurrentFrame = 0;
		mMaxTime = 0.0f;
	}
}

void VAnimatedSprite2D::SetTexture(VTexturePtr pPath)
{
	if (mTexture)
	{
		mTexture->LoadCallback.DisconnectSubscriber(this);
	}

	mTexture = pPath;

	if (mTexture)
	{
		mTexture->LoadCallback.Connect(this, [this](Any, const VEventArgs&)
		{
			mCacheDirty = true;
			mNeedUpdateMatrix = true;
			mNeedUpdateColor = true;
		});
	}

	if (mTexture->IsLoaded())
	{
		mCacheDirty = true;
	}

	mNeedUpdateMatrix = true;
	mNeedUpdateColor = true;
}

vec2 VAnimatedSprite2D::GetFrameSize() const
{
	return mFrameSize;
}

void VAnimatedSprite2D::SetFrameSize(const vec2& pFrameSize)
{
	mFrameSize.Set(std::max(0.0f, pFrameSize.x), std::max(0.0f, pFrameSize.y));
	mSize = mFrameSize;
	mMesh = VMeshBuilder::CreateQuad2D(0.0f, mSize, vec2(0.0f, 1.0f));
	mCacheDirty = true;
}

int VAnimatedSprite2D::GetRealFrameCount() const
{
	return mRealFrameCount;
}

int VAnimatedSprite2D::GetFrameCount() const
{
	return mFrameCount;
}

void VAnimatedSprite2D::SetFrameCount(int pFrameCount)
{
	mFrameCount = std::max(0, pFrameCount);
	mCacheDirty = true;
}

int VAnimatedSprite2D::GetFPS() const
{
	return mFPS;
}

void VAnimatedSprite2D::SetFPS(int pFPS)
{
	mFPS = std::max(0, pFPS);
	mCacheDirty = true;
}

bool VAnimatedSprite2D::IsLoop() const
{
	return mLoop;
}

void VAnimatedSprite2D::SetLoop(bool pLoop)
{
	mLoop = pLoop;
}

void VAnimatedSprite2D::Play()
{
	mActive = true;
}

void VAnimatedSprite2D::Pause()
{
	mActive = false;
}

void VAnimatedSprite2D::Stop()
{
	mActive = false;
	mCurrentFrame = 0;
	mCurrentTime = 0.0f;
}

float VAnimatedSprite2D::GetDuration() const
{
	return mMaxTime;
}

void VAnimatedSprite2D::Rewind(float pTime)
{
	mCurrentTime = std::max(0.0f, std::min(pTime, mMaxTime));
	
	if (mRealFrameCount > 0)
	{
		mCurrentFrame = int(floorf(mCurrentTime * float(mFPS))) % (mRealFrameCount - 1);
	}
}

bool VAnimatedSprite2D::IsActive() const
{
	return mActive;
}

void VAnimatedSprite2D::SetActive(bool pValue)
{
	if (pValue)
	{
		Stop();
		Play();
	}
	else
	{
		Stop();
	}
}

void VAnimatedSprite2D::UpdateMesh()
{
	if (mCurrentFrame >= int(mFrameCache.size()))
	{
		return;
	}

	const auto& frame = mFrameCache[mCurrentFrame];
	const auto& position = frame.Position();
	const auto& size = frame.Size();

	mMesh.VertexAt(0).UV.Set(position.x, position.y);
	mMesh.VertexAt(1).UV.Set(position.x, size.height);
	mMesh.VertexAt(2).UV.Set(size.width, size.height);
	mMesh.VertexAt(3).UV.Set(size.width, position.y);
}

void VAnimatedSprite2D::Update(float pDelta)
{
	VSprite2D::Update(pDelta);

	if (mIsPaused)
	{
		return;
	}

	UpdateFrameCache();

	if (!mActive || mFrameCount == 0 || mRealFrameCount == 0 || mFPS == 0)
	{
		UpdateMesh();
		return;
	}
		
	mCurrentTime += pDelta;

	const auto maxFrameIndex = mFrameCache.size() - 1;

	if (mCurrentTime > mMaxTime)
	{
		if (mLoop)
		{
			mCurrentTime -= mMaxTime;
		}
		else
		{
			mCurrentTime = mMaxTime;
			mCurrentFrame = maxFrameIndex;
			mActive = false;
			UpdateMesh();
			return;
		}
	}
	
	mCurrentFrame = int(floorf(mCurrentTime * float(mFPS))) % (maxFrameIndex + 1);

	UpdateMesh();
}