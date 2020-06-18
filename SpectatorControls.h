#pragma once
#pragma comment(lib, "pluginsdk.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/includes.h"
#include <chrono>

class SpectatorControls : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	std::shared_ptr<bool> enableRestoration;
	std::shared_ptr<bool> lockPosition;
	std::shared_ptr<bool> overrideZoom;
	std::shared_ptr<float> overrideZoomTransition;
	std::shared_ptr<float> overrideZoomSpeed;
	std::shared_ptr<float> overrideZoomMax;
	std::shared_ptr<float> overrideZoomMin;
	std::shared_ptr<float> zoomIncrementAmount;

	Vector  savedLocation = {0,0,100};
	Rotator savedRotation = {0,0,0};
	float   savedFOV = 90.f;

	int keyZoomIn;
	int keyZoomOut;

	std::string zoomInName;
	std::string zoomOutName;
	std::string zoomSpeedIncreaseName;
	std::string zoomSpeedDecreaseName;

	const double baseDelta = 1.0/60.0;
	std::chrono::steady_clock::time_point previousTime;
	struct ZoomInput
	{
		double amount;
		float speed;
		std::chrono::steady_clock::time_point inputTime;
	};
	std::vector<ZoomInput> zoomInputs;

public:
	virtual void onLoad();
	virtual void onUnload();

	void OnKeyChanged(int key, std::string cvarName);
	ServerWrapper GetCurrentGameState();

	void ResetCameraAll();
	void StoreCameraAll();

	void CameraTick();
	void LockPosition();
	void OnLockPositionChanged();
	void OverrideZoom(double delta);
	void ChangeZoomSpeed(bool increaseOrDecrease);
	void OnZoomEnabledChanged();

	void GetCameraAll();
	void SetCameraAll(std::vector<std::string> params);

	void GetCameraPosition();
	void SetCameraPosition(std::vector<std::string> params);

	void GetCameraRotation();
	void SetCameraRotation(std::vector<std::string> params);

	void GetCameraFOV();
	void SetCameraFOV(std::vector<std::string> params);

	void SetCameraFlyBall();
};