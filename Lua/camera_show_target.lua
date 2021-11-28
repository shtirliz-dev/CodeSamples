MetaData = {
	Name = "CameraShowTarget"
}

Properties = {
	Node = Any.new("camera"),
	TargetNode = Any.new(""),
	DelayBeforeCapture = Any.new(0.8),
	MoveTime = Any.new(1.0),
	ShowTime = Any.new(2.0),
	ReturnTime = Any.new(1.0),
	Listener = Any.new(""),
	EaseTypeFly = Any.new("EaseInOutCubic"),
	EaseTypeReturn = Any.new("EaseInOutCubic"),
	CanBeReturnedByTouch = Any.new(true)
}

local mCamera = nil
local mCameraActionNode = nil
local mCameraTarget = nil
local mCallback = nil
local mRoot = nil

function OnStart()
	mRoot = mOwner:GetRoot()
end

function OnStop()
	if mCallback ~= nil then
		mCallback(mSelf, mOwner, mCamera, "Reset")
	end
	
	mCallback = nil
	
	if mCameraActionNode ~= nil then
		mCameraActionNode:StopActions()
	end
	
	if mCamera ~= nil then
		mCamera:StopActions()
	end
	
	mCameraActionNode = nil
	mCamera = nil
	mCameraTarget = nil
	
	mSplitControlsValue = true
	mTimeScaleValue = 1.0
end

function ShowTargetWithNativeListener()
	local listenerName = Properties.Listener:Get("string")
	
	if #listenerName > 0 then
		local func = Utils.GetGlobalFunction(listenerName)
		
		if func ~= nil then
			ShowTarget(func)
			return
		end
	end
	
	ShowTarget()
end

function ShowTarget(cameraListener)
	local cameraName = Properties.Node:Get("string")
	
	if #cameraName == 0 then
		return false
	end
	
	mCamera = mRoot:FindNode("@" .. cameraName)
	
	if mCamera == nil then
		return false
	end
	
	mCameraActionNode = mCamera:FindNode("@action_helper")
	
	if mCameraActionNode == nil then
		return false
	end
	
	local cameraTargetName = Properties.TargetNode:Get("string")
	
	if #cameraTargetName == 0 then
		return false
	end
	
	mCameraTarget = mRoot:FindNode("@" .. cameraTargetName)
	
	if mCameraTarget == nil then
		return false
	end
	
	mCallback = cameraListener

	local split = mRoot:FindNode("@split")

	mCameraActionNode:StopActions()

	local canBeReturned = Properties.CanBeReturnedByTouch:Get("bool")
	local needTouchLayout = canBeReturned and Game.Runtime.CameraFlights[mOwner] ~= nil	
	
	if canBeReturned and not needTouchLayout then
		Game.Runtime.CameraFlights[mOwner] = 1
	end
	
	mCameraActionNode:StartAction(
		VSequence.Create(
			VCallFunction.Create(function()
				if cameraListener ~= nil then
					cameraListener(mSelf, mOwner, mCamera, "Begin")
				end
			end),
			VDelayTime.Create(Properties.DelayBeforeCapture:Get("float")),
			VFloatAction.Create(0.8, function(coeff) 
				VPhysics_SetTimeScale(1.0 - coeff)
			end),
			VCallFunction.Create(function()
				mCamera:StopActions()
				
				if split ~= nil then
					VSplit_SetControlsEnabled(split, false)
				end
				
				local posRef = mCamera:GetPosition()
				local oldPos = vec3.new(posRef.x, posRef.y, posRef.z)
				local pos = mCamera:GetParent():WorldToLocal(mCameraTarget:LocalToWorld(vec3.new(0, 0, 0)))
				
				local gameManager = GlobalStates["mGameManager"]
				
				if needTouchLayout and gameManager ~= nil then
					gameManager:PushCameraReleaseLayout(function()
						mCamera:StopActions()
						
						gameManager:RemoveCameraReleaseLayout()
						
						mCamera:StartAction(
							VSequence.Create(
								VCallFunction.Create(function()
									if cameraListener ~= nil then
										cameraListener(mSelf, mOwner, mCamera, "StartReturn")
									end
								end),
								VEaseAction.Create(
									VEaseType[Properties.EaseTypeReturn:Get("string")], VMoveTo.Create(Properties.ReturnTime:Get("float"), oldPos)
								),
								VSpawn.Create(
									VSequence.Create(
										VFloatAction.Create(0.2, function(coeff) 
											VPhysics_SetTimeScale(coeff)
										end),
										VCallFunction.Create(function() 
											if cameraListener ~= nil then
												cameraListener(mSelf, mOwner, mCamera, "EndReturn")
											end
										end)
									),
									VCallFunction.Create(function() 
										if split ~= nil then
											VSplit_SetControlsEnabled(split, true)
										end
									end)
								)
							)
						)
					end)
				end
				
				mCamera:StartAction(
					VSequence.Create(
						VCallFunction.Create(function()
							if cameraListener ~= nil then
								cameraListener(mSelf, mOwner, mCamera, "StartFly")
							end
						end),
						VEaseAction.Create(
							VEaseType[Properties.EaseTypeFly:Get("string")], VMoveTo.Create(Properties.MoveTime:Get("float"), pos)
						),
						VCallFunction.Create(function()
							if cameraListener ~= nil then
								cameraListener(mSelf, mOwner, mCamera, "EndFly")
							end
						end),
						VDelayTime.Create(Properties.ShowTime:Get("float")),
						VCallFunction.Create(function()
							if cameraListener ~= nil then
								cameraListener(mSelf, mOwner, mCamera, "StartReturn")
							end
							
							if needTouchLayout and gameManager ~= nil then
								gameManager:RemoveCameraReleaseLayout()
							end
						end),
						VEaseAction.Create(
							VEaseType[Properties.EaseTypeReturn:Get("string")], VMoveTo.Create(Properties.ReturnTime:Get("float"), oldPos)
						),
						VSpawn.Create(
							VSequence.Create(
								VFloatAction.Create(0.2, function(coeff) 
									VPhysics_SetTimeScale(coeff)
								end),
								VCallFunction.Create(function() 
									if cameraListener ~= nil then
										cameraListener(mSelf, mOwner, mCamera, "EndReturn")
									end
								end)
							),
							VCallFunction.Create(function() 
								if split ~= nil then
									VSplit_SetControlsEnabled(split, true)
								end
							end)
						)
					)
				)
			end)
		)
	)
	
	return true
end

function OnReset()
	OnStop()
	OnStart()
end