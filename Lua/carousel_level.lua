MetaData = {
	Name = "CarouselLevelScript"
}

local mGameManager = nil
local mContainer = nil
local mLevelNameLabel = nil
local mLevels = {}
local mSelectedLevel = nil
local mLevelTemplate = nil
local mTouchCallback = nil
local mHalfDistance = 0
local mWorldCenter = vec3.new(0)
local mLocalCenter = vec3.new(0)
local mOwnerPos = vec3.new(0)
local mOwnerSize = vec2.new(0)
local mOwnerParent = nil
local mLevelRootPos = vec3.new(0)
local mTouchLocalPos = vec3.new(0)
local mHittedPos = vec3.new(0)
local mMinScale = 0.25
local mLevelWidth = 0

local mFastSwipeTime = 0.25
local mFastSwipeOffsetPercent = 0.125
local mMoveBackTime = 0.5
local mDragOffsetCm = 0.25
local mInfo = nil

function SetGameManager(gameManager)
	mGameManager = gameManager
end

function GetInfo()
	return mInfo
end

function OnStart()
	mContainer = mOwner:FindNode("@items_container")
	mOwnerParent = mOwner:GetParent()
	
	local levelNodes = VUtils_GetChilds(mContainer)

	for i = 1, #levelNodes do
		local levelRoot = levelNodes[i]
		local levelName = levelRoot:GetName()
		local levelFillIndicator = CastToType(levelRoot:FindNode('@fill_indicator'), 'VSpriteFillable')
		local levelLockedNode = levelRoot:FindNode("@locked_item")
		local levelUnlockedNode = levelRoot:FindNode("@unlocked_item")

		table.insert(mLevels, {
			root = levelRoot,
			name = levelName,
			nameLoc = levelName .. "_title",
			fillIndicator = levelFillIndicator,
			lockedNode = levelLockedNode,
			unlockedNode = levelUnlockedNode
		})
	end

	mLevelNameLabel = CastToType(mOwner:GetParent():FindNode('@selected_level_name'), 'VTTFText')	
	mItemTemplate = mOwner:GetRoot():FindNode("@carousel_item_template") 
	
	if mItemTemplate ~= nil then
		mItemWidth = mItemTemplate:GetSize().x
	end
	
	UpdateLockedLevels()
	SetLevelsInactive()

	if #mLevels ~= 0 then
		SetSelectedLevel(mLevels[1])
	end

	local numPages = #mLevels
	
	local touch = {
		startPos = nil,
		startNodePos = nil,
		startPage = 0,
		curPage = 0,
		startTime = 0.0,
		initialHit = false,
		itemWidth = mItemWidth,
		numPages = numPages
	}
	
	mInfo = touch
	mTouchCallback = function(event)	
		Utils.ToLocalPointR(mOwner, event.mPosition, mTouchLocalPos)

		if event.mType == VTouchEventType.End or event.mType == VTouchEventType.Cancel then
			if touch.startPos == nil then
				return
			end			
			
			local diff = { 
				x = mTouchLocalPos.x - touch.startPos.x, 
				y = mTouchLocalPos.y - touch.startPos.y 
			}

			local touchStartPos = event:GetInfo():GetStartLocation()
			local touchCurrentPos = event:GetInfo():GetLocation()
			local touchOffset = (touchStartPos - touchCurrentPos):Length()
			local isDragged = VMath.InchesToCentimeters(touchOffset / gPlatform:GetScreenDPI()) > mDragOffsetCm
			local isHitted = touch.initialHit

			touch.startPos = nil
			touch.startNodePos = nil
			touch.initialHit = false

			if not isDragged and isHitted then            
				local index = Game.GetLevelIndex(mSelectedLevel.name)
				local levelModel = Game.GetLevelModel(index)

				if levelModel ~= nil and (levelModel.unlocked or Game.Cheats.OpenAll) then
					mGameManager:BlackOut(function()
						local introIndex = Game.GetLevelIndex(Game.General.IntroLevelName)
						index = returnIf(index > (introIndex + 1), index, introIndex)
						mGameManager:StartLevel(index, true)
					end)
				end
				
				return
			end

			local veryFast = (VUtils_Time() - touch.startTime) < mFastSwipeTime
			local veryShort = math.abs(diff.x) > (mItemWidth * mFastSwipeOffsetPercent)

			if touch.curPage == touch.startPage and veryFast and veryShort then
				if diff.x > 0.0 then
					touch.curPage = math.max(0, touch.curPage - 1)
				else
					touch.curPage = math.min(numPages - 1, touch.curPage + 1)
				end

				SetSelectedLevel(mLevels[touch.curPage + 1])
			end

			mContainer:StartAction(
				VEaseAction.Create(
					VEaseType.EaseOutBack, 
					VMoveTo.Create(mMoveBackTime, vec3.new(
						(-touch.curPage * mItemWidth), 
						0.0, 
						0.0
					))
				)
			)

			return
		end
		
		local hitted = Utils.HitTest(event.mPosition, mOwner)
		
		if hitted then
			mContainer:GetPositionR(mHittedPos)
			local sizeX = mItemWidth * #mLevels
			
			if touch.startPos == nil then
				mContainer:StopActions()
				touch.startPos = { x = mTouchLocalPos.x, y = mTouchLocalPos.y }
				touch.startNodePos = { x = mHittedPos.x, y = mHittedPos.y }
				touch.startPage = math.abs(math.floor(touch.startNodePos.x / mItemWidth + 0.5))
				touch.startTime = VUtils_Time()
				touch.initialHit = Utils.HitTest(event.mPosition, mSelectedLevel.root)
			else			
				local diff = { 
					x = mTouchLocalPos.x - touch.startPos.x, 
					y = mTouchLocalPos.y - touch.startPos.y 
				}

				mHittedPos.x = math.max(-sizeX + mItemWidth, math.min(touch.startNodePos.x + diff.x, 0.0))
				mHittedPos.y = 0
				mHittedPos.z = 0

				mContainer:SetPosition(mHittedPos)
				pos = mContainer:GetPosition()
				
				local page = math.abs(math.floor(mHittedPos.x / mItemWidth + 0.5))
				
				if page ~= touch.curPage then
					touch.curPage = page
					SetSelectedLevel(mLevels[touch.curPage + 1])
				end
			end
		end
	end
end

function ForceScrollToLevel(name, instant)
	for i = 1, #mLevels do
		if mLevels[i].name == name then
			mInfo.curPage = i - 1
			SetSelectedLevel(mLevels[i], instant)
			break
		end
	end
end

function OnStop()
	mContainer:SetPosition(vec3.new(0, 0, 0))
	mOwnerParent = nil
	mTouchCallback = nil
	mLevels = {}
end

function UpdateScales()
	for i = 1, #mLevels do
		local levelRoot = mLevels[i].root	
		levelRoot:GetPositionR(mLevelRootPos)
		Utils.ToCanvasPointR(levelRoot:GetParent(), mLevelRootPos, mLevelRootPos)
		local itemWorldX = mLevelRootPos.x
		local dist = math.abs(itemWorldX - mWorldCenter.x)
		local coeff = VUtils_Clamp(1.0 - dist / mHalfDistance + mMinScale, mMinScale, 1.0)
		levelRoot:SetScaleXY(coeff, coeff)
	end
end

function UpdateLockedLevels()
	for i = 1, #mLevels do
		local level = mLevels[i]
		local index = Game.GetLevelIndex(level.name)
		local levelModel = Game.GetLevelModel(index)
		if levelModel ~= nil and level.lockedNode ~= nil and level.unlockedNode ~= nil then
			local locked = levelModel.unlocked or Game.Cheats.OpenAll
			level.lockedNode:SetVisible(not locked)
			level.unlockedNode:SetVisible(locked)
		end
	end
end

function ShowSelection(instant)
	if mSelectedLevel == nil then
		return
	end 
	local indicator = mSelectedLevel.fillIndicator
	indicator:StopActions()
	if not instant then
		indicator:StartAction(
			VFloatAction.Create(0.5, function(value) 
				indicator:SetFillValue(value) 
			end)
		)
	else
		indicator:SetFillValue(1.0)
	end
end

function HideSelection(instant)
	if mSelectedLevel == nil then
		return
	end 
	local indicator = mSelectedLevel.fillIndicator
	indicator:StopActions()
	if not instant then
		indicator:StartAction(
			VFloatAction.Create(0.5, function(value) 
				indicator:SetFillValue(1.0 - value) 
			end)
		)
	else
		indicator:SetFillValue(0.0)
	end
end

function SetSelectedLevel(level, instant)
	if mSelectedLevel == level then
		return
	end

	local noAnim = instant or false

	if mSelectedLevel ~= nil then
		local indicator = mSelectedLevel.fillIndicator
		indicator:StopActions()
		if noAnim then
			indicator:SetFillValue(0.0) 
		else
			indicator:StartAction(
				VFloatAction.Create(0.5, function(value) 
					indicator:SetFillValue(1.0 - value) 
				end)
			)
		end
	end

	mSelectedLevel = level

	if mSelectedLevel ~= nil then
		local indicator = mSelectedLevel.fillIndicator
		mLevelNameLabel:SetLocalizationId(mSelectedLevel.nameLoc)
		indicator:StopActions()
		if noAnim then
			indicator:SetFillValue(1.0) 
		else
			indicator:StartAction(
				VFloatAction.Create(0.5, function(value) 
					indicator:SetFillValue(value) 
				end)
			)
		end
	else
		mLevelNameLabel:SetLocalizationId("")
	end
end

function SetLevelsInactive()
	if #mLevels ~= 0 then
		mLevelNameLabel:SetLocalizationId(mLevels[1].nameLoc)
		for i = 1, #mLevels do
			local fillIndicator = mLevels[i].fillIndicator
			if fillIndicator ~= nil then
				fillIndicator:SetFillValue(0)
			end
		end
	end
end

function OnUpdate(pDelta)	
	if not IsVisibleRecursive(mOwner) or #mLevels == 0 then
		return
	end
	
	mOwner:GetSizeInPointsR(mOwnerSize)

	mLocalCenter.x = mOwnerSize.x * 0.5
	mLocalCenter.y = mOwnerSize.y * 0.5
	mLocalCenter.z = 0
	
	Utils.ToCanvasPointR(mOwner, mLocalCenter, mWorldCenter)
	mOwner:GetPositionR(mOwnerPos)
	Utils.ToCanvasPointR(mOwnerParent, mOwnerPos, mOwnerPos)
	mHalfDistance = math.abs(mOwnerPos.x - mWorldCenter.x)

	UpdateScales()
end

function OnTouchEvent(pEvent)
	if mGameManager ~= nil and mGameManager:IsTransitionActive() then
		return
	end
	if not IsEditorMode() and mTouchCallback ~= nil then
		mTouchCallback(pEvent)
	end
end

function OnKeyboardEvent(event)
	if true then
		return
	end
	if mGameManager ~= nil and mGameManager:IsTransitionActive() then
		return
	end	
	local selected = mGameManager:GetSelected()
	if selected == nil or selected.node ~= mContainer then
		return
	end
	local nextPage = -1
	if event.mType == 1 and event.mKey == 141 then
		nextPage = mInfo.curPage + 1
		if nextPage >= #mLevels then
			nextPage = #mLevels - 1
		end
	end
	if event.mType == 1 and event.mKey == 139 then
		nextPage = mInfo.curPage - 1
		if nextPage < 0 then
			nextPage = 0
		end
	end
	if mInfo.curPage ~= nextPage and nextPage >= 0 then
		mContainer:StopActions()
		mContainer:SetPosition(vec3.new(mInfo.curPage * -mItemWidth, 0, 0))
		mContainer:StartAction(
			VEaseAction.Create(
				VEaseType.EaseOutBack, 
				VMoveTo.Create(mMoveBackTime, vec3.new(
					(nextPage * -mItemWidth), 
					0.0, 
					0.0
				))
			)
		)
		mInfo.curPage = nextPage
		SetSelectedLevel(mLevels[mInfo.curPage + 1])
	end
	if event.mType == 1 and event.mKey == 13 then
		local index = Game.GetLevelIndex(mSelectedLevel.name)
		local levelModel = Game.GetLevelModel(index)

		if levelModel ~= nil and (levelModel.unlocked or Game.Cheats.OpenAll) then
			mGameManager:BlackOut(function()
				local introIndex = Game.GetLevelIndex(Game.General.IntroLevelName)
				index = returnIf(index > (introIndex + 1), index, introIndex)
				mGameManager:StartLevel(index, true)
			end)
		end
	end
end

function OnInputActionEvent(event)
	if mGameManager ~= nil and mGameManager:IsTransitionActive() then
		return
	end	
	local selected = mGameManager:GetSelected()
	if selected == nil or selected.node ~= mContainer then
		return
	end
	local nextPage = -1
	if event.mAction == "right" and event.mBool then
		nextPage = mInfo.curPage + 1
		if nextPage >= #mLevels then
			nextPage = #mLevels - 1
		end
	end
	if event.mAction == "left" and event.mBool then
		nextPage = mInfo.curPage - 1
		if nextPage < 0 then
			nextPage = 0
		end
	end
	if mInfo.curPage ~= nextPage and nextPage >= 0 then
		mContainer:StopActions()
		mContainer:SetPosition(vec3.new(mInfo.curPage * -mItemWidth, 0, 0))
		mContainer:StartAction(
			VEaseAction.Create(
				VEaseType.EaseOutBack, 
				VMoveTo.Create(mMoveBackTime, vec3.new(
					(nextPage * -mItemWidth), 
					0.0, 
					0.0
				))
			)
		)
		mInfo.curPage = nextPage
		SetSelectedLevel(mLevels[mInfo.curPage + 1])
	end
	if event.mAction == "confirm" and event.mBool then
		local index = Game.GetLevelIndex(mSelectedLevel.name)
		local levelModel = Game.GetLevelModel(index)

		if levelModel ~= nil and (levelModel.unlocked or Game.Cheats.OpenAll) then
			mGameManager:BlackOut(function()
				local introIndex = Game.GetLevelIndex(Game.General.IntroLevelName)
				index = returnIf(index > (introIndex + 1), index, introIndex)
				mGameManager:StartLevel(index, true)
			end)
		end
	end
end