#include "SpectatorControls.h"

using namespace std;

BAKKESMOD_PLUGIN(SpectateCameraShortcuts, "Tools for spectators", "1.3", PLUGINTYPE_SPECTATOR)

#define GET_DURATION(x, y) chrono::duration<double> x = chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - y)

void SpectateCameraShortcuts::onLoad()
{
	//CVARS
	enableRestoration = make_shared<bool>(false);
	lockPosition = make_shared<bool>(false);
	overrideZoom = make_shared<bool>(false);
	overrideZoomTransition = make_shared<float>(0.f);
	overrideZoomSpeed = make_shared<float>(0.f);
	overrideZoomMax = make_shared<float>(0.f);
	overrideZoomMin = make_shared<float>(0.f);
	zoomIncrementAmount = make_shared<float>(0.f);

	cvarManager->registerCvar("Spectate_EnableRestoration", "1", "Saves camera values before a goal replay and restores values after goal replay", true, true, 0, true, 1).bindTo(enableRestoration);
	cvarManager->registerCvar("Spectate_LockPosition", "0", "Locks the camera to the last specified position", true, true, 0, true, 1).bindTo(lockPosition);
	cvarManager->registerCvar("Spectate_OverrideZoom", "0", "Overrides zoom controls with analog triggers", true, true, 0, true, 1).bindTo(overrideZoom);
	cvarManager->getCvar("Spectate_LockPosition").addOnValueChanged(bind(&SpectateCameraShortcuts::OnLockPositionChanged, this));
	cvarManager->getCvar("Spectate_OverrideZoom").addOnValueChanged(bind(&SpectateCameraShortcuts::OnZoomEnabledChanged, this));

	cvarManager->registerCvar("Spectate_OverrideZoom_Transition_Time", "0.5", "Averages zoom input", true, true, 0, true, 2).bindTo(overrideZoomTransition);
	cvarManager->registerCvar("Spectate_OverrideZoom_Speed", "45", "Zoom speed", true, true, 0, true, 150).bindTo(overrideZoomSpeed);
	cvarManager->registerCvar("Spectate_OverrideZoom_Max", "140", "Zoom max", true, true, 5, true, 170).bindTo(overrideZoomMax);
	cvarManager->registerCvar("Spectate_OverrideZoom_Min", "20", "Zoom min", true, true, 5, true, 170).bindTo(overrideZoomMin);
	cvarManager->registerCvar("Spectate_OverrideZoom_Speed_Increment_Amount", "20", "Zoom speed increment amount", true, true, 0, true, 100).bindTo(zoomIncrementAmount);

	//NOTIFIERS
	cvarManager->registerNotifier("SpectateGetCamera",         [this](std::vector<string> params){GetCameraAll();}, "Print camera data to add to list of options", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateSetCamera",         [this](std::vector<string> params){SetCameraAll(params);}, "Set camera position location and rotation. Usage: SpectateSetCamera <loc X> <loc Y> <loc Z> <rot X (in degrees)> <rot y> <rot z> <FOV>", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateGetCameraPosition", [this](std::vector<string> params){GetCameraPosition();}, "Print camera position", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateSetCameraPosition", [this](std::vector<string> params){SetCameraPosition(params);}, "Set camera position. Usage: SpectateSetCameraPosition <loc X> <loc Y> <loc Z>", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateGetCameraRotation", [this](std::vector<string> params){GetCameraRotation();}, "Print camera rotation", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateSetCameraRotation", [this](std::vector<string> params){SetCameraRotation(params);}, "Set camera rotation. Usage: SpectateSetCameraRotation <rot X (in degrees)> <rot y> <rot z>", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateGetCameraFOV",      [this](std::vector<string> params){GetCameraFOV();}, "Print camera FOV", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateSetCameraFOV",      [this](std::vector<string> params){SetCameraFOV(params);}, "Set camera FOV. Usage: SpectateSetCameraFOV <FOV>", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateIncreaseZoomSpeed", [this](std::vector<string> params){ChangeZoomSpeed(true);}, "Increases zoom speed by 20", PERMISSION_ALL);
	cvarManager->registerNotifier("SpectateDecreaseZoomSpeed", [this](std::vector<string> params){ChangeZoomSpeed(false);}, "Decreases zoom speed by 20", PERMISSION_ALL);

	//HOOK EVENTS
	gameWrapper->HookEvent("Function TAGame.Camera_Replay_TA.UpdateCamera", std::bind(&SpectateCameraShortcuts::CameraTick, this));
	gameWrapper->HookEvent("Function TAGame.Team_TA.EventScoreUpdated", bind(&SpectateCameraShortcuts::StoreCameraAll, this));
	gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState", bind(&SpectateCameraShortcuts::ResetCameraAll, this));
	
	//KEYBINDS
	zoomInName = "Spectate_Keybind_ZoomIn";
	zoomOutName = "Spectate_Keybind_ZoomOut";
	zoomSpeedDecreaseName = "Spectate_Keybind_ZoomSpeed_Increase";
	zoomSpeedIncreaseName = "Spectate_Keybind_ZoomSpeed_Decrease";
	cvarManager->registerCvar(zoomInName, "XboxTypeS_RightTrigger", "Zoom in keybind", true);
	cvarManager->registerCvar(zoomOutName, "XboxTypeS_LeftTrigger", "Zoom out keybind", true);
	cvarManager->registerCvar(zoomSpeedIncreaseName, "XboxTypeS_LeftThumbStick", "Zoom speed decrease keybind", true);
	cvarManager->registerCvar(zoomSpeedDecreaseName, "XboxTypeS_RightThumbStick", "Zoom speed increase keybind", true);
	cvarManager->getCvar(zoomInName).addOnValueChanged(bind(&SpectateCameraShortcuts::OnKeyChanged, this, 0, zoomInName));
	cvarManager->getCvar(zoomOutName).addOnValueChanged(bind(&SpectateCameraShortcuts::OnKeyChanged, this, 1, zoomOutName));
	cvarManager->getCvar(zoomSpeedIncreaseName).addOnValueChanged(bind(&SpectateCameraShortcuts::OnKeyChanged, this, 2, zoomSpeedIncreaseName));
	cvarManager->getCvar(zoomSpeedDecreaseName).addOnValueChanged(bind(&SpectateCameraShortcuts::OnKeyChanged, this, 3, zoomSpeedDecreaseName));

	keyZoomIn = gameWrapper->GetFNameIndexByString(cvarManager->getCvar(zoomInName).getStringValue());
	keyZoomOut = gameWrapper->GetFNameIndexByString(cvarManager->getCvar(zoomOutName).getStringValue());
}
void SpectateCameraShortcuts::onUnload(){}
void SpectateCameraShortcuts::OnKeyChanged(int key, string cvarName)
{
	string stringValue = cvarManager->getCvar(cvarName).getStringValue();

	if(key == 0) keyZoomIn = gameWrapper->GetFNameIndexByString(stringValue);
	else if(key == 1) keyZoomOut = gameWrapper->GetFNameIndexByString(stringValue);
	else if(key == 2) cvarManager->setBind(stringValue, "SpectateIncreaseZoomSpeed");
	else if(key == 3) cvarManager->setBind(stringValue, "SpectateDecreaseZoomSpeed");
	else cvarManager->log("Invalid key index");
}
ServerWrapper SpectateCameraShortcuts::GetCurrentGameState()
{
	if(gameWrapper->IsInReplay())
		return gameWrapper->GetGameEventAsReplay().memory_address;
	else if(gameWrapper->IsInOnlineGame())
		return gameWrapper->GetOnlineGame();
	else
		return gameWrapper->GetGameEventAsServer();
}

//Restoration
void SpectateCameraShortcuts::StoreCameraAll()
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
void SpectateCameraShortcuts::ResetCameraAll()
{
	if(!(*enableRestoration)) return;

	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull() && gameWrapper->GetLocalCar().IsNull())
	{
		camera.SetLocation(savedLocation);
		camera.SetRotation(savedRotation);
		camera.SetFOV(savedFOV);
		camera.SetbLockedFOV(0);
	}
}

//Camera Tick
void SpectateCameraShortcuts::CameraTick()
{
	//chrono::duration<double> dur = chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - previousTime);
	GET_DURATION(dur, previousTime);
	previousTime = chrono::steady_clock::now();
	double delta = dur.count() / baseDelta;//Get delta time for consistent zoom override

	if(*lockPosition) LockPosition();
	if(*overrideZoom) OverrideZoom(delta);
}
void SpectateCameraShortcuts::LockPosition()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(camera.IsNull()) return;

	camera.SetLocation(savedLocation);
}
void SpectateCameraShortcuts::OnLockPositionChanged()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(camera.IsNull()) return;

	if(*lockPosition)
	{
		savedLocation = camera.GetLocation();
	}
}
void SpectateCameraShortcuts::OverrideZoom(double delta)
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(camera.IsNull()) return;

	//Add new input value
	double input = 0.0;
	if(gameWrapper->IsKeyPressed(keyZoomOut))
	{
		input += (1.0 * delta);
	}
	if(gameWrapper->IsKeyPressed(keyZoomIn))
	{
		input -= (1.0 * delta);
	}

	//Refresh inputs list
	zoomInputs.push_back(ZoomInput{input, *overrideZoomSpeed, chrono::steady_clock::now()});
	bool haveUpdatedInputs = false;
	while(!haveUpdatedInputs)
	{
		//chrono::duration<double> oldestInputDuration = chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - zoomInputs[0].inputTime);
		GET_DURATION(oldestInputDuration, zoomInputs[0].inputTime);
		if(oldestInputDuration.count() > (double)(*overrideZoomTransition))
			zoomInputs.erase(zoomInputs.begin());
		else
			haveUpdatedInputs = true;
	}

	//Generate zoom amount from averaged inputs and chosen zoom speed multiplier
	double totalInput = 0;
	float totalSpeed = 0;
	for(int i=0; i<zoomInputs.size(); i++)
	{
		totalInput += zoomInputs[i].amount;
		totalSpeed += zoomInputs[i].speed;
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
void SpectateCameraShortcuts::ChangeZoomSpeed(bool increaseOrDecrease)
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
void SpectateCameraShortcuts::OnZoomEnabledChanged()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(camera.IsNull()) return;

	if(!(*overrideZoom))
		camera.SetbLockedFOV(false);
}

//ALL
void SpectateCameraShortcuts::GetCameraAll()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull())
	{
		cvarManager->log("Position: " + to_string(camera.GetLocation().X) + ", " + to_string(camera.GetLocation().Y) + ", " + to_string(camera.GetLocation().Z));
		cvarManager->log("Rotation Degrees: " + to_string(camera.GetRotation().Pitch / 182.044449) + ", " + to_string(camera.GetRotation().Yaw / 182.044449) + ", " + to_string(camera.GetRotation().Roll / 182.044449));
		cvarManager->log("FOV: " + to_string(camera.GetFOV()));
		cvarManager->log("copypaste for UI: 0|BUTTONNAME|SpectateSetCamera " + to_string(camera.GetLocation().X) + " " + to_string(camera.GetLocation().Y) + " " + to_string(camera.GetLocation().Z) + " " + to_string(camera.GetRotation().Pitch / 182.044449) + " " + to_string(camera.GetRotation().Yaw / 182.044449) + " " + to_string(camera.GetRotation().Roll / 182.044449) + " " + to_string(camera.GetFOV()));
		cvarManager->log("copypaste for keybind: bind KEY \"SpectateSetCamera " + to_string(camera.GetLocation().X) + " " + to_string(camera.GetLocation().Y) + " " + to_string(camera.GetLocation().Z) + " " + to_string(camera.GetRotation().Pitch / 182.044449) + " " + to_string(camera.GetRotation().Yaw / 182.044449) + " " + to_string(camera.GetRotation().Roll / 182.044449) + " " + to_string(camera.GetFOV()) + "\"");
	}
}
void SpectateCameraShortcuts::SetCameraAll(std::vector<string> params)
{
	if(gameWrapper->GetLocalCar().IsNull())
	{
		//float LocX, LocY, LocZ, FOV;
		//int RotP, RotY, RotR;

		CameraWrapper camera = gameWrapper->GetCamera();
		if(!camera.IsNull())
		{
			//Location
			if(params.size() > 1)
				savedLocation.X = stof(params.at(1));
			else
				savedLocation.X = camera.GetLocation().X;
			if(params.size() > 2)
				savedLocation.Y = stof(params.at(2));
			else
				savedLocation.Y = camera.GetLocation().Y;
			if(params.size() > 3)
				savedLocation.Z = stof(params.at(3));
			else
				savedLocation.Z = camera.GetLocation().Z;

			//Rotation
			if(params.size() > 4)
				savedRotation.Pitch = (int)(stof(params.at(4)) * 182.044449);
			else
				savedRotation.Pitch = camera.GetRotation().Pitch;
			if(params.size() > 5)
				savedRotation.Yaw = (int)(stof(params.at(5)) * 182.044449);
			else
				savedRotation.Yaw = camera.GetRotation().Yaw;
			if(params.size() > 6)
				savedRotation.Roll = (int)(stof(params.at(6)) * 182.044449);
			else
				savedRotation.Roll = camera.GetRotation().Roll;

			//FOV
			if(params.size() > 7)
				savedFOV = stof(params.at(7));
			else
				savedFOV = camera.GetFOV();


			//Set values
			camera.SetLocation(savedLocation);
			camera.SetRotation(savedRotation);
			camera.SetFOV(savedFOV);
			camera.SetbLockedFOV(0);
		}
	}
	else
		cvarManager->log("Cannot change camera while in control of a car!");
}

//POSITION
void SpectateCameraShortcuts::GetCameraPosition()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull())
	{
		cvarManager->log("Position: " + to_string(camera.GetLocation().X) + ", " + to_string(camera.GetLocation().Y) + ", " + to_string(camera.GetLocation().Z));
		cvarManager->log("copypaste for UI: 0|BUTTONNAME|SpectateSetCameraPosition " + to_string(camera.GetLocation().X) + " " + to_string(camera.GetLocation().Y) + " " + to_string(camera.GetLocation().Z));
		cvarManager->log("copypaste for keybind: bind KEY \"SpectateSetCameraPosition " + to_string(camera.GetLocation().X) + " " + to_string(camera.GetLocation().Y) + " " + to_string(camera.GetLocation().Z) + "\"");
	}
}
void SpectateCameraShortcuts::SetCameraPosition(std::vector<string> params)
{
	if(gameWrapper->GetLocalCar().IsNull())
	{
		CameraWrapper camera = gameWrapper->GetCamera();
		if(!camera.IsNull())
		{
			//Location
			if(params.size() > 1)
				savedLocation.X = stof(params.at(1));
			else
				savedLocation.X = camera.GetLocation().X;
			if(params.size() > 2)
				savedLocation.Y = stof(params.at(2));
			else
				savedLocation.Y = camera.GetLocation().Y;
			if(params.size() > 3)
				savedLocation.Z = stof(params.at(3));
			else
				savedLocation.Z = camera.GetLocation().Z;

			//Set values
			camera.SetLocation(savedLocation);
		}
	}
	else
		cvarManager->log("Cannot change camera while in control of a car!");
}


//ROTATION
void SpectateCameraShortcuts::GetCameraRotation()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull())
	{
		cvarManager->log("Rotation Degrees: " + to_string(camera.GetRotation().Pitch / 182.044449) + ", " + to_string(camera.GetRotation().Yaw / 182.044449) + ", " + to_string(camera.GetRotation().Roll / 182.044449));
		cvarManager->log("copypaste for UI: 0|BUTTONNAME|SpectateSetCameraRotation " + to_string(camera.GetRotation().Pitch / 182.044449) + " " + to_string(camera.GetRotation().Yaw / 182.044449) + " " + to_string(camera.GetRotation().Roll / 182.044449));
		cvarManager->log("copypaste for keybind: bind KEY \"SpectateSetCameraRotation " + to_string(camera.GetRotation().Pitch / 182.044449) + " " + to_string(camera.GetRotation().Yaw / 182.044449) + " " + to_string(camera.GetRotation().Roll / 182.044449) + "\"");
	}
}
void SpectateCameraShortcuts::SetCameraRotation(std::vector<string> params)
{
	if(gameWrapper->GetLocalCar().IsNull())
	{
		CameraWrapper camera = gameWrapper->GetCamera();
		if(!camera.IsNull())
		{
			//Rotation
			if(params.size() > 1)
				savedRotation.Pitch = (int)(stof(params.at(1)) * 182.044449);
			else
				savedRotation.Pitch = camera.GetRotation().Pitch;
			if(params.size() > 2)
				savedRotation.Yaw = (int)(stof(params.at(2)) * 182.044449);
			else
				savedRotation.Yaw = camera.GetRotation().Yaw;
			if(params.size() > 3)
				savedRotation.Roll = (int)(stof(params.at(3)) * 182.044449);
			else
				savedRotation.Roll = camera.GetRotation().Roll;

			//Set values
			camera.SetRotation(savedRotation);
		}
	}
	else
		cvarManager->log("Cannot change camera while in control of a car!");
}

//FOV
void SpectateCameraShortcuts::GetCameraFOV()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if(!camera.IsNull())
	{
		cvarManager->log("FOV: " + to_string(camera.GetFOV()));
		cvarManager->log("copypaste for UI: 0|BUTTONNAME|SpectateSetCameraFOV " + to_string(camera.GetFOV()));
		cvarManager->log("copypaste for keybind: bind KEY \"SpectateSetCameraFOV " + to_string(camera.GetFOV()) + "\"");
	}
}
void SpectateCameraShortcuts::SetCameraFOV(std::vector<string> params)
{
	if(gameWrapper->GetLocalCar().IsNull())
	{
		CameraWrapper camera = gameWrapper->GetCamera();
		if(!camera.IsNull())
		{
			//FOV
			if(params.size() > 1)
				savedFOV = stof(params.at(1));
			else
				savedFOV = camera.GetFOV();

			//Set values
			camera.SetFOV(savedFOV);
			camera.SetbLockedFOV(0);
		}
	}
	else
		cvarManager->log("Cannot change camera while in control of a car!");
}
