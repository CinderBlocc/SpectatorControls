#include "SpectatorControls.h"

BAKKESMOD_PLUGIN(SpectatorControls, "Tools for spectators", "1.5", PLUGINTYPE_SPECTATOR)

#define GET_DURATION(x, y) std::chrono::duration<float> x = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - y)

void SpectatorControls::onLoad()
{
	//CVARS
	enableRestoration = std::make_shared<bool>(false);
	lockPosition = std::make_shared<bool>(false);
    lockVerticalMovement = std::make_shared<bool>(false);
	overrideZoom = std::make_shared<bool>(false);
	overrideZoomTransition = std::make_shared<float>(0.f);
	overrideZoomSpeed = std::make_shared<float>(0.f);
	overrideZoomMax = std::make_shared<float>(0.f);
	overrideZoomMin = std::make_shared<float>(0.f);
	zoomIncrementAmount = std::make_shared<float>(0.f);
    rotationSmoothDuration = std::make_shared<float>(0.f);
    rotationSmoothMultiplier = std::make_shared<float>(0.f);

	cvarManager->registerCvar("Spectate_EnableRestoration", "1", "Saves camera values before a goal replay and restores values after goal replay", true, true, 0, true, 1).bindTo(enableRestoration);
	cvarManager->registerCvar("Spectate_LockPosition", "0", "Locks the camera to the last specified position", true, true, 0, true, 1).bindTo(lockPosition);
    cvarManager->registerCvar("Spectate_LockVertical", "0", "Prevents vertical movement from the camera", true, true, 0, true, 1).bindTo(lockVerticalMovement);
	cvarManager->registerCvar("Spectate_OverrideZoom", "0", "Overrides zoom controls with analog triggers", true, true, 0, true, 1).bindTo(overrideZoom);
	cvarManager->getCvar("Spectate_LockPosition").addOnValueChanged(std::bind(&SpectatorControls::OnLockPositionChanged, this));
	cvarManager->getCvar("Spectate_OverrideZoom").addOnValueChanged(std::bind(&SpectatorControls::OnZoomEnabledChanged, this));

	cvarManager->registerCvar("Spectate_OverrideZoom_Transition_Time", "0.5", "Averages zoom input", true, true, 0, true, 2).bindTo(overrideZoomTransition);
	cvarManager->registerCvar("Spectate_OverrideZoom_Speed", "45", "Zoom speed", true, true, 0, true, 150).bindTo(overrideZoomSpeed);
	cvarManager->registerCvar("Spectate_OverrideZoom_Max", "140", "Zoom max", true, true, 5, true, 170).bindTo(overrideZoomMax);
	cvarManager->registerCvar("Spectate_OverrideZoom_Min", "20", "Zoom min", true, true, 5, true, 170).bindTo(overrideZoomMin);
	cvarManager->registerCvar("Spectate_OverrideZoom_Speed_Increment_Amount", "20", "Zoom speed increment amount", true, true, 0, true, 100).bindTo(zoomIncrementAmount);

    cvarManager->registerCvar("Spectate_SmoothRotation_Transition_Time", "0", "Averages rotation input", true, true, 0, true, 2).bindTo(rotationSmoothDuration);
    cvarManager->registerCvar("Spectate_SmoothRotation_Multiplier", "1", "Multiplier for rotation inputs. Used for slowing down rotation speed", true, true, 0, true, 1).bindTo(rotationSmoothMultiplier);

	//NOTIFIERS
	cvarManager->registerNotifier("SpectateGetCamera",         [this](std::vector<std::string> params){GetCameraAll();}, "Print camera data to add to list of options", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateSetCamera",         [this](std::vector<std::string> params){SetCameraAll(params);}, "Set camera position location and rotation. Usage: SpectateSetCamera <loc X> <loc Y> <loc Z> <rot X (in degrees)> <rot y> <rot z> <FOV>", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateGetCameraPosition", [this](std::vector<std::string> params){GetCameraPosition();}, "Print camera position", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateSetCameraPosition", [this](std::vector<std::string> params){SetCameraPosition(params);}, "Set camera position. Usage: SpectateSetCameraPosition <loc X> <loc Y> <loc Z>", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateGetCameraRotation", [this](std::vector<std::string> params){GetCameraRotation();}, "Print camera rotation", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateSetCameraRotation", [this](std::vector<std::string> params){SetCameraRotation(params);}, "Set camera rotation. Usage: SpectateSetCameraRotation <rot X (in degrees)> <rot y> <rot z>", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateGetCameraFOV",      [this](std::vector<std::string> params){GetCameraFOV();}, "Print camera FOV", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateSetCameraFOV",      [this](std::vector<std::string> params){SetCameraFOV(params);}, "Set camera FOV. Usage: SpectateSetCameraFOV <FOV>", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateIncreaseZoomSpeed", [this](std::vector<std::string> params){ChangeZoomSpeed(true);}, "Increases zoom speed by 20", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateDecreaseZoomSpeed", [this](std::vector<std::string> params){ChangeZoomSpeed(false);}, "Decreases zoom speed by 20", PERMISSION_ALL);

	cvarManager->registerNotifier("SpectateSetCameraFlyBall", [this](std::vector<std::string> params){SetCameraFlyBall();}, "Force the camera into flycam with the ball as the target", PERMISSION_ALL);
    cvarManager->registerNotifier("SpectateSetCameraFlyNoTarget", [this](std::vector<std::string> params){SetCameraFlyNoTarget();}, "Force the camera into flycam with no target", PERMISSION_ALL);

    cvarManager->registerNotifier("SpectateUnlockFOV", [this](std::vector<std::string> params){UnlockFOV();}, "Unlock camera FOV. Will reenable scroll wheel zooming", PERMISSION_ALL);

	//HOOK EVENTS
	gameWrapper->HookEvent("Function TAGame.Camera_Replay_TA.UpdateCamera", std::bind(&SpectatorControls::CameraTick, this));
    gameWrapper->HookEvent("Function TAGame.PlayerInput_TA.PlayerInput", std::bind(&SpectatorControls::PlayerInputTick, this));
	gameWrapper->HookEvent("Function TAGame.Team_TA.EventScoreUpdated", std::bind(&SpectatorControls::StoreCameraAll, this));
	gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState", std::bind(&SpectatorControls::ResetCameraAll, this));
	
	//KEYBINDS
	zoomInName = "Spectate_Keybind_ZoomIn";
	zoomOutName = "Spectate_Keybind_ZoomOut";
	zoomSpeedDecreaseName = "Spectate_Keybind_ZoomSpeed_Increase";
	zoomSpeedIncreaseName = "Spectate_Keybind_ZoomSpeed_Decrease";
	cvarManager->registerCvar(zoomInName, "XboxTypeS_RightTrigger", "Zoom in keybind", true);
	cvarManager->registerCvar(zoomOutName, "XboxTypeS_LeftTrigger", "Zoom out keybind", true);
	cvarManager->registerCvar(zoomSpeedIncreaseName, "XboxTypeS_LeftThumbStick", "Zoom speed decrease keybind", true);
	cvarManager->registerCvar(zoomSpeedDecreaseName, "XboxTypeS_RightThumbStick", "Zoom speed increase keybind", true);
	cvarManager->getCvar(zoomInName).addOnValueChanged(bind(&SpectatorControls::OnKeyChanged, this, KeybindChange::KEY_ZoomIn, zoomInName));
	cvarManager->getCvar(zoomOutName).addOnValueChanged(bind(&SpectatorControls::OnKeyChanged, this, KeybindChange::KEY_ZoomOut, zoomOutName));
	cvarManager->getCvar(zoomSpeedIncreaseName).addOnValueChanged(bind(&SpectatorControls::OnKeyChanged, this, KeybindChange::KEY_IncreaseZoomSpeed, zoomSpeedIncreaseName));
	cvarManager->getCvar(zoomSpeedDecreaseName).addOnValueChanged(bind(&SpectatorControls::OnKeyChanged, this, KeybindChange::KEY_DecreaseZoomSpeed, zoomSpeedDecreaseName));

	keyZoomIn = gameWrapper->GetFNameIndexByString(cvarManager->getCvar(zoomInName).getStringValue());
	keyZoomOut = gameWrapper->GetFNameIndexByString(cvarManager->getCvar(zoomOutName).getStringValue());
}
void SpectatorControls::onUnload(){}
void SpectatorControls::OnKeyChanged(KeybindChange changedKey, std::string cvarName)
{
	std::string stringValue = cvarManager->getCvar(cvarName).getStringValue();

    switch(changedKey)
    {
        case KeybindChange::KEY_ZoomIn:
            keyZoomIn = gameWrapper->GetFNameIndexByString(stringValue);
            break;
        case KeybindChange::KEY_ZoomOut :
            keyZoomOut = gameWrapper->GetFNameIndexByString(stringValue);
            break;
        case KeybindChange::KEY_IncreaseZoomSpeed :
            cvarManager->setBind(stringValue, "SpectateIncreaseZoomSpeed");
            break;
        case KeybindChange::KEY_DecreaseZoomSpeed :
            cvarManager->setBind(stringValue, "SpectateDecreaseZoomSpeed");
            break;
        default :
            cvarManager->log("Invalid key index");
    }
}
ServerWrapper SpectatorControls::GetCurrentGameState()
{
	if(gameWrapper->IsInReplay())
		return gameWrapper->GetGameEventAsReplay().memory_address;
	else if(gameWrapper->IsInOnlineGame())
		return gameWrapper->GetOnlineGame();
	else
		return gameWrapper->GetGameEventAsServer();
}
bool SpectatorControls::IsValidState()
{
    if(!gameWrapper->GetLocalCar().IsNull())
	{
        cvarManager->log("Cannot change camera while in control of a car!");
        return false;
    }

	CameraWrapper camera = gameWrapper->GetCamera();
	if(camera.IsNull())
	{
        cvarManager->log("Cannot change camera. Camera is null.");
        return false;
    }

    return true;
}

//Restoration
void SpectatorControls::StoreCameraAll()
{
	if(!(*enableRestoration)) return;

	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull())
	{
		savedLocation = camera.GetLocation();
		savedRotation = camera.GetRotation();
		savedFOV = camera.GetFOV();
	}
}
void SpectatorControls::ResetCameraAll()
{
	if(!(*enableRestoration) || !gameWrapper->GetLocalCar().IsNull()) return;

	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull() && gameWrapper->GetLocalCar().IsNull())
	{
		camera.SetLocation(savedLocation);
        camera.SetRotation(savedRotation);
        camera.SetFOV(savedFOV);
	}
}

//Camera and PlayInput Ticks
void SpectatorControls::CameraTick()
{
	GET_DURATION(dur, previousTime);
	previousTime = std::chrono::steady_clock::now();
	float delta = dur.count() / baseDelta;

	OverrideZoom(delta);
}
void SpectatorControls::PlayerInputTick()
{
    GetCameraInputs();
    SmoothRotationInputs();
    LockPosition();
}
void SpectatorControls::GetCameraInputs()
{
    PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
    if(pc.IsNull()) { return; }

	currentCameraInputs.Forward = pc.GetAForward();
	currentCameraInputs.Strafe  = pc.GetAStrafe();
	currentCameraInputs.Up      = pc.GetAUp();
	currentCameraInputs.Turn    = pc.GetATurn();
	currentCameraInputs.LookUp  = pc.GetALookUp();
}
void SpectatorControls::LockPosition()
{
    if(!gameWrapper->GetLocalCar().IsNull()) return;

	CameraWrapper camera = gameWrapper->GetCamera();
	if(camera.IsNull()) return;

    
    PlayerControllerWrapper controller = gameWrapper->GetPlayerController();
    if(controller.IsNull()) { return; }
	
    if(*lockPosition)
    {
	    camera.SetLocation(savedLocation);
        controller.SetAForward(0);
	    controller.SetAStrafe(0);
	    controller.SetATurn(0);
	    controller.SetALookUp(0);
    }
	
    if(*lockPosition || *lockVerticalMovement)
    {
        controller.SetAUp(0);
    }
}
void SpectatorControls::OnLockPositionChanged()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(camera.IsNull()) return;

	if(*lockPosition)
	{
		savedLocation = camera.GetLocation();
	}
}
void SpectatorControls::SmoothRotationInputs()
{
    PlayerControllerWrapper controller = gameWrapper->GetPlayerController();
    if(!gameWrapper->GetLocalCar().IsNull() || controller.IsNull()) { return; }

    float lookUpFinal = currentCameraInputs.LookUp;
    float turnFinal = currentCameraInputs.Turn;

    if(*rotationSmoothDuration > 0)
    {
        //Refresh lookUp inputs list
	    rotationInputs.push_back(RotationInput{currentCameraInputs.LookUp, currentCameraInputs.Turn, std::chrono::steady_clock::now()});
	    while(!rotationInputs.empty())
	    {
		    GET_DURATION(oldestInputDuration, rotationInputs[0].inputTime);
		    if(oldestInputDuration.count() > *rotationSmoothDuration)
			    rotationInputs.erase(rotationInputs.begin());
		    else
			    break;
	    }
        float lookUpTotal = 0, turnTotal = 0;
        for(const auto &input : rotationInputs)
        {
            lookUpTotal += input.lookUpAmount;
            turnTotal += input.turnAmount;
        }
        float lookUpAverage = lookUpTotal / static_cast<float>(rotationInputs.size());
        float turnAverage = turnTotal / static_cast<float>(rotationInputs.size());
        lookUpFinal = lookUpAverage;
        turnFinal = turnAverage;
    }

    if(*rotationSmoothMultiplier < 1)
    {
        lookUpFinal *= *rotationSmoothMultiplier;
        turnFinal *= *rotationSmoothMultiplier;
    }
    
    //Set input values
    controller.SetALookUp(lookUpFinal);
    controller.SetATurn(turnFinal);
}
void SpectatorControls::OverrideZoom(float delta)
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(!*overrideZoom || camera.IsNull() || !gameWrapper->GetLocalCar().IsNull()) return;

    if(camera.GetCameraState().find("ReplayFly") == std::string::npos)
    {
        if(camera.GetCameraState().find("CameraState_Car_TA") != std::string::npos)
        {
            camera.SetFOV(camera.GetCameraSettings().FOV);
        }
        camera.SetbLockedFOV(false);
        return;
    }

	//Add new input value
	float input = 0.f;
	if(gameWrapper->IsKeyPressed(keyZoomOut))
	{
        if(cvarManager->getCvar(zoomOutName).getStringValue() == "XboxTypeS_LeftTrigger" && currentCameraInputs.Up < 0)
        {
            input += (-currentCameraInputs.Up * delta);
        }
        else
        {
    		input += (1.f * delta);
        }
	}
	if(gameWrapper->IsKeyPressed(keyZoomIn))
	{
        if(cvarManager->getCvar(zoomInName).getStringValue() == "XboxTypeS_RightTrigger" && currentCameraInputs.Up > 0)
        {
            input += (-currentCameraInputs.Up * delta);
        }
        else
        {
    		input += (-1.f * delta);
        }
	}

	//Refresh inputs list
	zoomInputs.push_back(ZoomInput{input, *overrideZoomSpeed, std::chrono::steady_clock::now()});
	bool haveUpdatedInputs = false;
	while(!haveUpdatedInputs)
	{
		GET_DURATION(oldestInputDuration, zoomInputs[0].inputTime);
		if(oldestInputDuration.count() > *overrideZoomTransition)
			zoomInputs.erase(zoomInputs.begin());
		else
			haveUpdatedInputs = true;
	}

	//Generate zoom amount from averaged inputs and chosen zoom speed multiplier
	double totalInput = 0;
	float totalSpeed = 0;
	for(const auto &zoomInput : zoomInputs)
	{
		totalInput += zoomInput.amount;
		totalSpeed += zoomInput.speed;
	}
	float avgInput = (float)totalInput / (float)zoomInputs.size();
	float avgSpeed = totalSpeed / (float)zoomInputs.size();

	float currFOV = camera.GetFOV();
	float newFOV = currFOV + (avgInput * (avgSpeed * (float)baseDelta));

	float FOVmax = *overrideZoomMax;
	float FOVmin = *overrideZoomMin;
	if(FOVmin > FOVmax)
	{
		FOVmax = *overrideZoomMin;
		FOVmin = *overrideZoomMax;
	}

	if(newFOV < FOVmin) newFOV = FOVmin;
	if(newFOV > FOVmax) newFOV = FOVmax;
	camera.SetFOV(newFOV);
}
void SpectatorControls::ChangeZoomSpeed(bool increaseOrDecrease)
{
	CVarWrapper zoomSpeed = cvarManager->getCvar("Spectate_OverrideZoom_Speed");
	float curr = zoomSpeed.getFloatValue();

	if(increaseOrDecrease)
	{
		curr += *zoomIncrementAmount;
		if(curr > 150) curr = 150;
	}
	else
	{
		curr -= *zoomIncrementAmount;
		if(curr < 0) curr = 0;
	}

	zoomSpeed.setValue(curr);
}
void SpectatorControls::OnZoomEnabledChanged()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(camera.IsNull()) return;

	if(!(*overrideZoom))
		camera.SetbLockedFOV(false);
}

void SpectatorControls::UnlockFOV()
{
    CameraWrapper camera = gameWrapper->GetCamera();
    if(!camera.IsNull())
    {
        camera.SetLockedFOV(false);
    }
}

//ALL
void SpectatorControls::GetCameraAll()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull())
	{
		cvarManager->log("Position: " + std::to_string(camera.GetLocation().X) + ", " + std::to_string(camera.GetLocation().Y) + ", " + std::to_string(camera.GetLocation().Z));
		cvarManager->log("Rotation Degrees: " + std::to_string(camera.GetRotation().Pitch / 182.044449) + ", " + std::to_string(camera.GetRotation().Yaw / 182.044449) + ", " + std::to_string(camera.GetRotation().Roll / 182.044449));
		cvarManager->log("FOV: " + std::to_string(camera.GetFOV()));
		cvarManager->log("copypaste for UI: 0|BUTTONNAME|SpectateSetCamera " + std::to_string(camera.GetLocation().X) + " " + std::to_string(camera.GetLocation().Y) + " " + std::to_string(camera.GetLocation().Z) + " " + std::to_string(camera.GetRotation().Pitch / 182.044449) + " " + std::to_string(camera.GetRotation().Yaw / 182.044449) + " " + std::to_string(camera.GetRotation().Roll / 182.044449) + " " + std::to_string(camera.GetFOV()));
		cvarManager->log("copypaste for keybind: bind KEY \"SpectateSetCamera " + std::to_string(camera.GetLocation().X) + " " + std::to_string(camera.GetLocation().Y) + " " + std::to_string(camera.GetLocation().Z) + " " + std::to_string(camera.GetRotation().Pitch / 182.044449) + " " + std::to_string(camera.GetRotation().Yaw / 182.044449) + " " + std::to_string(camera.GetRotation().Roll / 182.044449) + " " + std::to_string(camera.GetFOV()) + "\"");
	}
}
void SpectatorControls::SetCameraAll(std::vector<std::string> params)
{
	if(!IsValidState()) { return; }

	CameraWrapper camera = gameWrapper->GetCamera();
    
	//Location
    savedLocation.X = (params.size() > 1) ? stof(params.at(1)) : camera.GetLocation().X;
    savedLocation.Y = (params.size() > 2) ? stof(params.at(2)) : camera.GetLocation().Y;
    savedLocation.Z = (params.size() > 3) ? stof(params.at(3)) : camera.GetLocation().Z;

	//Rotation
    savedRotation.Pitch = (params.size() > 4) ? (int)(stof(params.at(4)) * 182.044449) : camera.GetRotation().Pitch;
    savedRotation.Yaw   = (params.size() > 5) ? (int)(stof(params.at(5)) * 182.044449) : camera.GetRotation().Yaw;
    savedRotation.Roll  = (params.size() > 6) ? (int)(stof(params.at(6)) * 182.044449) : camera.GetRotation().Roll;

	//FOV
	savedFOV = (params.size() > 7) ? stof(params.at(7)) : camera.GetFOV();

	//Set values	
    camera.SetLocation(savedLocation);
    camera.SetRotation(savedRotation);
    camera.SetFOV(savedFOV);
}

//POSITION
void SpectatorControls::GetCameraPosition()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull())
	{
		cvarManager->log("Position: " + std::to_string(camera.GetLocation().X) + ", " + std::to_string(camera.GetLocation().Y) + ", " + std::to_string(camera.GetLocation().Z));
		cvarManager->log("copypaste for UI: 0|BUTTONNAME|SpectateSetCameraPosition " + std::to_string(camera.GetLocation().X) + " " + std::to_string(camera.GetLocation().Y) + " " + std::to_string(camera.GetLocation().Z));
		cvarManager->log("copypaste for keybind: bind KEY \"SpectateSetCameraPosition " + std::to_string(camera.GetLocation().X) + " " + std::to_string(camera.GetLocation().Y) + " " + std::to_string(camera.GetLocation().Z) + "\"");
	}
}
void SpectatorControls::SetCameraPosition(std::vector<std::string> params)
{
    if(!IsValidState()) { return; }

	CameraWrapper camera = gameWrapper->GetCamera();

	//Location
    savedLocation.X = (params.size() > 1) ? stof(params.at(1)) : camera.GetLocation().X;
    savedLocation.Y = (params.size() > 2) ? stof(params.at(2)) : camera.GetLocation().Y;
    savedLocation.Z = (params.size() > 3) ? stof(params.at(3)) : camera.GetLocation().Z;

	//Set values
	camera.SetLocation(savedLocation);
}

//ROTATION
void SpectatorControls::GetCameraRotation()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull())
	{
		cvarManager->log("Rotation Degrees: " + std::to_string(camera.GetRotation().Pitch / 182.044449) + ", " + std::to_string(camera.GetRotation().Yaw / 182.044449) + ", " + std::to_string(camera.GetRotation().Roll / 182.044449));
		cvarManager->log("copypaste for UI: 0|BUTTONNAME|SpectateSetCameraRotation " + std::to_string(camera.GetRotation().Pitch / 182.044449) + " " + std::to_string(camera.GetRotation().Yaw / 182.044449) + " " + std::to_string(camera.GetRotation().Roll / 182.044449));
		cvarManager->log("copypaste for keybind: bind KEY \"SpectateSetCameraRotation " + std::to_string(camera.GetRotation().Pitch / 182.044449) + " " + std::to_string(camera.GetRotation().Yaw / 182.044449) + " " + std::to_string(camera.GetRotation().Roll / 182.044449) + "\"");
	}
}
void SpectatorControls::SetCameraRotation(std::vector<std::string> params)
{
    if(!IsValidState()) { return; }

	CameraWrapper camera = gameWrapper->GetCamera();

	//Rotation
    savedRotation.Pitch = (params.size() > 1) ? (int)(stof(params.at(1)) * 182.044449) : camera.GetRotation().Pitch;
    savedRotation.Yaw   = (params.size() > 2) ? (int)(stof(params.at(2)) * 182.044449) : camera.GetRotation().Yaw;
    savedRotation.Roll  = (params.size() > 3) ? (int)(stof(params.at(3)) * 182.044449) : camera.GetRotation().Roll;

	//Set values
    camera.SetRotation(savedRotation);
}

//FOV
void SpectatorControls::GetCameraFOV()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull())
	{
		cvarManager->log("FOV: " + std::to_string(camera.GetFOV()));
		cvarManager->log("copypaste for UI: 0|BUTTONNAME|SpectateSetCameraFOV " + std::to_string(camera.GetFOV()));
		cvarManager->log("copypaste for keybind: bind KEY \"SpectateSetCameraFOV " + std::to_string(camera.GetFOV()) + "\"");
	}
}
void SpectatorControls::SetCameraFOV(std::vector<std::string> params)
{
    if(!IsValidState()) { return; }

    CameraWrapper camera = gameWrapper->GetCamera();

	//FOV
	savedFOV = (params.size() > 1) ? stof(params.at(1)) : camera.GetFOV();

	//Set values
    camera.SetFOV(savedFOV);
}

//FORCE FLYCAM
void SpectatorControls::SetCameraFlyBall()
{
	if(gameWrapper->GetLocalCar().IsNull() && !gameWrapper->GetCamera().IsNull())
	{
        gameWrapper->GetCamera().SetFlyCamBallTargetMode();
	}
	else
    {
		cvarManager->log("Cannot change camera while in control of a car!");
    }
}
void SpectatorControls::SetCameraFlyNoTarget()
{
    if(gameWrapper->GetLocalCar().IsNull() && !gameWrapper->GetCamera().IsNull())
	{
        gameWrapper->GetCamera().SetFlyCamBallTargetMode();
        gameWrapper->GetCamera().SetFocusActor("");
	}
	else
    {
		cvarManager->log("Cannot change camera while in control of a car!");
    }
}
