MetaData = {
	Name = "UIControls"
}

local mRightUI = {}
local mLeftUI = {}
local mSplit = nil
local mControls = nil
local mLControl = nil
local mRControl = nil
local mRoot = nil
local mPadding = (0.1 * gPlatform:GetScreenDPI()) / GetDesignResolutionScale()
local mControlPosition = vec3.new(0)
local mRectCenter = vec2.new(0)

function CreateLeft()
	local lRect = mLControl:GetAreaRect("left")
	local dRect = mLControl:GetAreaRect("down")
	local btnSize = MinRectSide(lRect)
	local btnSizeSmall = MinRectSide(dRect)
	local scaleBig = (btnSize - mPadding) / btnSize
	local scaleSmall = (btnSizeSmall - mPadding) / btnSizeSmall

	local left = CreateButton({
		size = btnSize,
		sprite = 'sprites/controls/left.png',
		opacity = 0.25,
		scale = scaleBig,
		area = "left", 
		control = mLControl,
		gravity = "down",
		fullRectSize = lRect:Size()
	})

	local right = CreateButton({
		size = btnSize,
		sprite = 'sprites/controls/right.png',
		opacity = 0.25,
		scale = scaleBig,
		area = "right", 
		control = mLControl,
		gravity = "down",
		fullRectSize = lRect:Size()
	})

	local down = CreateButton({
		size = btnSizeSmall,
		sprite = 'sprites/controls/down.png',
		opacity = 0.25,
		scale = scaleSmall,
		area = "down", 
		control = mLControl
	})

	mRoot:AttachChild(down.holder)
	mRoot:AttachChild(left.holder)
	mRoot:AttachChild(right.holder)

	mLeftUI.left = left
	mLeftUI.right = right
	mLeftUI.down = down
end

function CreateRight()
	local tBtnSize = MinRectSide(mRControl:GetAreaRect("throw"))
	local gBtnSize = MinRectSide(mRControl:GetAreaRect("grab"))
	local jBtnSize = MinRectSide(mRControl:GetAreaRect("jump"))
	local tScale = (tBtnSize - mPadding) / tBtnSize
	local gScale = (gBtnSize - mPadding) / gBtnSize
	local jScale = (jBtnSize - mPadding) / jBtnSize

	local throw = CreateButton({
		size = tBtnSize,
		sprite = 'sprites/controls/T.png',
		opacity = 0.25,
		scale = tScale,
		area = "throw", 
		control = mRControl
	})

	local jump = CreateButton({
		size = jBtnSize,
		sprite = 'sprites/controls/J.png',
		opacity = 0.25,
		scale = jScale,
		area = "jump", 
		control = mRControl
	})

	local jumpAlt = CreateButton({
		size = jBtnSize,
		sprite = 'sprites/controls/H_alt.png',
		opacity = 0.25,
		scale = jScale,
		area = "jump", 
		control = mRControl
	})

	local grab = CreateButton({
		size = gBtnSize,
		sprite = 'sprites/controls/G.png',
		opacity = 0.25,
		scale = gScale,
		area = "grab", 
		control = mRControl
	})

	mRoot:AttachChild(jump.holder)
	mRoot:AttachChild(jumpAlt.holder)
	mRoot:AttachChild(throw.holder)
	mRoot:AttachChild(grab.holder)

	mRightUI.jumpAlt = jumpAlt
	mRightUI.jump = jump
	mRightUI.throw = throw
	mRightUI.grab = grab
end

function CreateRoot()
	mRoot = VUtils_CreateLayoutNode()
	mRoot:SetPositionXPercent(true)
	mRoot:SetPositionYPercent(true)
	mRoot:SetPosition(vec3.new(0, -1, 0))
	mRoot:SetName("ui_controls")
	mOwner:InsertChild(mRoot, 0)
	mLControl:UpdateLayout()
	mRControl:UpdateLayout()
	CreateLeft()
	CreateRight()
	UpdateLayout()
	FadeIn()
end

function FadeIn()
	local applyEffect = function(node)
		local prev = node.back:GetOpacity()
		node.back:SetOpacity(0)
		node.back:StartAction(VFadeTo.Create(0.25, prev))
	end
	for k, v in pairs(mLeftUI) do
		applyEffect(v)
	end
	for k, v in pairs(mRightUI) do
		applyEffect(v)
	end
end

function Cleanup()
	if mRoot ~= nil then
		mOwner:DetachChild(mRoot)
	end
	mRightUI = {}
	mLeftUI = {}
end

function SetSplit(split)
	mSplit = split
	mSplitComponent = CastToType(mSplit:GetComponent("Split"), "VComponentSplit")
	mControls = CastToType(mSplit:GetComponent('SplitTouchControls'), 'VComponentSplitTouchControls')
	mLControl = mControls:GetLeftControl()
	mRControl = mControls:GetRightControl()
end

function OnStart()
	CreateRoot()
	NotificationManager_SubscribeComponent(mSelf, "SceneViewResized")
end

function OnStop()
	Cleanup()	
	NotificationManager_UnsubscribeComponent(mSelf, "SceneViewResized")
end

function OnNotificationReceive(pNotificationName, pData)
	if pNotificationName == "SceneViewResized" then
		UpdateLayout()
	end
end

function SetButtonPressed(button, value)
	if value ~= button.pressed or button.pressed == nil then
		if value then
			button.back:SetScale(button.backScalePressed)
		else
			button.back:SetScale(button.backScaleDefault)
		end
		button.pressed = value
	end
end

local mCache = {
	touch = nil,
	isMoving = nil,
	isSqueeze = nil,
	moveDir = nil,
	spikesEnabled = nil,
	grabWaveEnabled = nil,
	canActivateSpikes = nil,
	touchInJumpArea = nil,
	touchInThrowArea = nil
}

function OnUpdate(delta)
	if IsPadConnected() then
		mRoot:SetVisible(false)
	else
		mRoot:SetVisible(true)
	end

	mCache.touch = mRControl:GetTouch()
	mCache.isMoving = mSplitComponent:IsMoving()
	mCache.isSqueeze = mSplitComponent:IsSqueezeEnabled()
	mCache.moveDir = mSplitComponent:GetMovingDirection()
	mCache.spikesEnabled = mSplitComponent:IsSpikesEnabled()
	mCache.grabWaveEnabled = mSplitComponent:IsGrabWaveEnabled()
	mCache.canActivateSpikes = mCache.touch ~= nil and mRControl:CanActivateSpikes()
	mCache.touchInJumpArea = mCache.touch ~= nil and mRControl:IsTouchInJumpArea()
	mCache.touchInThrowArea = mCache.touch ~= nil and mRControl:IsTouchInThrowArea()
	mCache.grabMode = mCache.touch ~= nil and (mRControl:GetMode() == 1)

	SetButtonPressed(mRightUI.grab, mCache.grabWaveEnabled or mCache.canActivateSpikes)
	SetButtonPressed(mRightUI.jump, mCache.touchInJumpArea)
	SetButtonPressed(mRightUI.jumpAlt, mCache.touchInJumpArea or mCache.grabMode)
	SetButtonPressed(mRightUI.throw, mCache.touchInThrowArea)
	
	mRightUI.grab.holder:SetVisible(not mCache.grabMode)
	mRightUI.throw.holder:SetVisible(mCache.grabMode)
	mRightUI.jumpAlt.holder:SetVisible(mCache.grabMode)
	mRightUI.jump.holder:SetVisible(not mCache.grabMode)

	if mCache.isMoving then
		SetButtonPressed(mLeftUI.left, mCache.moveDir < 0.0)
		SetButtonPressed(mLeftUI.right, mCache.moveDir >= 0.0)
	else
		SetButtonPressed(mLeftUI.left, false)
		SetButtonPressed(mLeftUI.right, false)
	end

	SetButtonPressed(mLeftUI.down, mCache.isSqueeze)
end

function RectCenter(rect)
	local pos = rect:Position()
	local size = rect:Size()
	mRectCenter.x = pos.x + size.x * 0.5
	mRectCenter.y = pos.y + size.y * 0.5
	
	return mRectCenter
end

function MinRectSide(rect)
	local size = rect:Size()
	return math.min(size.x, size.y)
end

function UpdateControlPosition(button, offsetX)
	local rect = button.control:GetAreaRect(button.area)
	local rectCenter = RectCenter(rect)
	mControlPosition.x = rectCenter.x - offsetX
	mControlPosition.y = rectCenter.y
	mControlPosition.z = 0
	
	button.holder:SetPositionXY(mControlPosition.x, mControlPosition.y)
end

function UpdateLayout()
	mLControl:UpdateLayout()
	mRControl:UpdateLayout()
	local offset = mLControl:GetVisibleAreaOffsetX()
	for k, v in pairs(mLeftUI) do
		UpdateControlPosition(v, offset)
	end
	for k, v in pairs(mRightUI) do
		UpdateControlPosition(v, offset)
	end
end

function CreateButton(params)
	local holder = VUtils_CreateLayoutNode()

	holder:SetName("button_" .. params.area)
	holder:SetIsAnchorPercent(true)
	holder:SetAnchor(Vec2.Middle)

	if params.fullRectSize ~= nil then
		holder:SetSize(params.fullRectSize)
	else
		holder:SetSize(vec2.new(params.size, params.size))
	end

	local back = VUtils_CreateSprite()

	back:SetTextureSync(params.sprite)
	back:SetIsAnchorPercent(true)
	back:SetOpacity(params.opacity)
	back:SetName("sprite_" .. params.area)

	local halfSize = params.size * 0.5
	local backSize = back:GetSize()
	local backScale = vec2.new(
		params.size / backSize.x * params.scale, 
		params.size / backSize.y * params.scale
	)

	back:SetAnchor(Vec2.Middle)
	back:SetScale(backScale)

	if params.gravity == "down" then
		local scaledWidth = backSize.x * backScale.x
		local padding = (params.fullRectSize.x - scaledWidth) * 0.5
		back:SetPosition(vec3.new(halfSize, -params.fullRectSize.y + padding + scaledWidth * 0.5, 0))
	else
		back:SetPosition(vec3.new(halfSize, -halfSize, 0))
	end

	holder:AttachChild(back)

	return {
		holder = holder,
		back = back,
		backScale = backScale,
		backScalePressed = vec2.new(0.85, 0.85) * backScale.x,
		backScaleDefault = Vec2.One * backScale.x,
		backPos = back:GetPosition(),
		area = params.area,
		control = params.control
	}
end