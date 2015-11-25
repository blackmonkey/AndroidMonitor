#pragma once

#include "Device.h"

class AdbDetectRequest;
class AdbKeyRequest;
class AdbPointerRequest;
class AdbScreenshotRequest;

class AdbDevice : public Device
{
public:
	AdbDevice(string sn, string stateStr);
	~AdbDevice(void);
	int getType(void);
	string getName(void);
	string getDescription(void);
	void setPointerDeivce(string dev);
	void pointerDown(int x, int y);
	void pointerMove(int x, int y);
	void pointerUp(void);
	void keyStrike(int keycode);

private:
	static const char TAG[12];
	string name;
	AdbDetectRequest* detectRequest;
	AdbKeyRequest* keyRequest;
	AdbPointerRequest* pointerRequest;
	AdbScreenshotRequest* screenshotRequest;

	void requestScreenshot(void);
	void showScreenshot(LPVOID image);
	void initAddrinfo(void);
};
