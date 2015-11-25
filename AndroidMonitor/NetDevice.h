#pragma once

#include "Device.h"

class NetScreenshotRequest;

class NetDevice : public Device
{
public:
	NetDevice(DWORD ip, int port);
	~NetDevice(void);
	int getType(void);
	string getName(void);
	string getDescription(void);
	void pointerDown(int x, int y);
	void pointerMove(int x, int y);
	void pointerUp(void);
	void keyStrike(int keycode);

private:
	static const char TAG[12];
	string ip;
	string port;
	NetScreenshotRequest* screenshotRequest;

	void requestScreenshot(void);
	void showScreenshot(LPVOID image);
	void initAddrinfo(void);
};
