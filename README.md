# SpectatorControls
This plugin provides additional control for Rocket League broadcast observers.
Here are the current features:

1. Position Hotkeys: You can bind position/rotation/FOV into one hotkey, or choose to separate them. Those commands work as follows:
+ SpectateSetCamera <loc X> <loc Y> <loc Z> <rot X (in degrees)> <rot y> <rot z> <FOV>
+ SpectateSetCameraPosition <loc X> <loc Y> <loc Z>
+ SpectateSetCameraRotation <rot X (in degrees)> <rot y> <rot z>
+ SpectateSetCameraFOV <FOV>
2. Zoom Override Controls: These settings control the smoothness, FOV limits, and speed of the zoom. It also allows rebinding of the zoom keys.
3. Lock Position: Freezes the camera in its current position, or in the new position you specify with the "SpectateSetCameraPosition" command
4. Camera Restoration: Returns the camera to the position it was in after a goal replay finishes (camera position is stored the moment the goal is scored)
