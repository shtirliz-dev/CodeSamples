#pragma once

#include <Render/Texture.h>
#include <Render/Mesh.h>
#include <Nodes/Sprite2D.h>

class VAnimatedSprite2D : public VSprite2D
{
	MANAGED_TYPE(VAnimatedSprite2D)

private:
	std::vector<VRectF> mFrameCache;
	vec2 mFrameSize = 0.0f;
	int mFrameCount = 0;
	int mRealFrameCount = 0;
	int mFPS = 0;
	bool mLoop = true;
	bool mActive = true;
	int mCurrentFrame = 0;
	float mMaxTime = 0.0f;
	float mCurrentTime = 0.0f;
	bool mCacheDirty = false;

protected:
	void UpdateFrameCache();
	void UpdateMesh();

public:
	VAnimatedSprite2D();

	void Update(float pDelta) override;
	void SetTexture(VTexturePtr pPath)override;

	VNode2D* PickNode(const vec2& pPoint, bool pRecursive) override;
	VNode2D* PickNode3D(const vec3& pPointStart, const vec3& pPointEnd,
		std::vector<VNodePickingData>& pResults,
		bool pRecursive = true,
		const int pDepth = 0) override;

	vec2 GetFrameSize() const;
	void SetFrameSize(const vec2& pFrameSize);

	int GetFrameCount() const;
	void SetFrameCount(int pFrameCount);

	int GetRealFrameCount() const;

	int GetFPS() const;
	void SetFPS(int pFPS);

	bool IsLoop() const;
	void SetLoop(bool pLoop);
	
	void Play();
	void Pause();
	void Stop();

	float GetDuration() const;
	void Rewind(float pTime);

	bool IsActive() const;
	void SetActive(bool pValue);

	static void ReflectType(VMetaData& pMeta);
	static void RegisterTypeLua(VLuaState* pLua);
};