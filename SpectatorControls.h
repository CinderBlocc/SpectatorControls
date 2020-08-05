#pragma once
#pragma comment(lib, "pluginsdk.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/includes.h"
#include <chrono>

enum class KeybindChange
{
    KEY_ZoomIn = 0,
    KEY_ZoomOut,
    KEY_IncreaseZoomSpeed,
    KEY_DecreaseZoomSpeed
};

class SpectatorControls : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	std::shared_ptr<bool> enableRestoration;
	std::shared_ptr<bool> lockPosition;
    std::shared_ptr<bool> lockVerticalMovement;
	std::shared_ptr<bool> overrideZoom;
	std::shared_ptr<float> overrideZoomTransition;
	std::shared_ptr<float> overrideZoomSpeed;
	std::shared_ptr<float> overrideZoomMax;
	std::shared_ptr<float> overrideZoomMin;
	std::shared_ptr<float> zoomIncrementAmount;
    std::shared_ptr<float> rotationSmoothDuration;
    std::shared_ptr<float> rotationSmoothMultiplier;

	Vector  savedLocation = {0,0,100};
	Rotator savedRotation = {0,0,0};
	float   savedFOV = 90.f;

	int keyZoomIn;
	int keyZoomOut;

	std::string zoomInName;
	std::string zoomOutName;
	std::string zoomSpeedIncreaseName;
	std::string zoomSpeedDecreaseName;

	const float baseDelta = 1.f/60.f;
	std::chrono::steady_clock::time_point previousTime;
	struct ZoomInput
	{
		double amount;
		float speed;
		std::chrono::steady_clock::time_point inputTime;
	};
	std::vector<ZoomInput> zoomInputs;

    struct RotationInput
    {
        float lookUpAmount;
        float turnAmount;
        std::chrono::steady_clock::time_point inputTime;
    };
    std::vector<RotationInput> rotationInputs;

    struct CameraInputs
    {
        float Forward;
        float Strafe;
        float Up;
        float Turn;
        float LookUp;
    };

    CameraInputs currentCameraInputs;

public:
	void onLoad() override;
	void onUnload() override;

	void OnKeyChanged(KeybindChange changedKey, std::string cvarName);
	ServerWrapper GetCurrentGameState();
    bool IsValidState();

	void ResetCameraAll();
	void StoreCameraAll();

	void CameraTick();
    void PlayerInputTick();
    void GetCameraInputs();
	void LockPosition();
	void OnLockPositionChanged();
    void SmoothRotationInputs();
	void OverrideZoom(float delta);
	void ChangeZoomSpeed(bool increaseOrDecrease);
	void OnZoomEnabledChanged();

    void UnlockFOV();

	void GetCameraAll();
	void SetCameraAll(std::vector<std::string> params);

	void GetCameraPosition();
	void SetCameraPosition(std::vector<std::string> params);

	void GetCameraRotation();
	void SetCameraRotation(std::vector<std::string> params);

	void GetCameraFOV();
	void SetCameraFOV(std::vector<std::string> params);

	void SetCameraFlyBall();
    void SetCameraFlyNoTarget();
};