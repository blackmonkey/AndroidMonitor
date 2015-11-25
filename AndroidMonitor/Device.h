#pragma once

#include <string>
#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

using namespace std;

class Device
{
public:
	/** Device state */
	static const int UNKNOWN    = -1;
	static const int BOOTLOADER = 0;
	static const int OFFLINE    = 1;
	static const int ONLINE     = 2;
	static const int READY      = 3;
	static const int CONNECTING = 4;

	/** Device type */
	static const int ADB_DEVICE = 1;
	static const int NET_DEVICE = 2;
	static const int BT_DEVICE  = 3;

	Device(void);
	~Device(void);
	LPADDRINFO getAddrinfo(void);
	bool isMonitoring(void);
	void setMonitoring(BOOL enable);
	int getState(void);
	void setState(int state);
	void setOutputWnd(HWND hDlg, HWND hCtrl);
	void startScreenshot(void);
	void stopScreenshot(void);
	void onScreenshot(LPVOID image);

	virtual int getType(void) = 0;
	virtual string getName(void) = 0;
	virtual string getDescription(void) = 0;
	virtual void pointerDown(int x, int y) = 0;
	virtual void pointerMove(int x, int y) = 0;
	virtual void pointerUp(void) = 0;
	virtual void keyStrike(int keycode) = 0;

protected:
	LPADDRINFO addinfo; // The address information of ethernet device
	HWND screenCtrl;
	HWND screenDlg;

	static int parseState(string stateStr);
	string getStateStr(void);

	virtual void requestScreenshot(void) = 0;
	virtual void showScreenshot(LPVOID image) = 0;
	virtual void initAddrinfo(void) = 0;

private:
	static const char TAG[8];
	bool monitoring;
	bool show;   // Whether show the screenshot
	int state;
};
