#include "StdAfx.h"
#include "Device.h"

const char Device::TAG[8] = "Device";

Device::Device(void)
{
	setMonitoring(false);
	setState(UNKNOWN);
	setOutputWnd(NULL, NULL);
	stopScreenshot();
}

Device::~Device(void)
{
	if (addinfo) {
		freeaddrinfo(addinfo);
		addinfo = NULL;
	}
	setMonitoring(false);
	stopScreenshot();
	setOutputWnd(NULL, NULL);
	setState(UNKNOWN);
}

LPADDRINFO Device::getAddrinfo(void)
{
	return addinfo;
}

bool Device::isMonitoring(void)
{
	return monitoring;
}

void Device::setMonitoring(BOOL enable)
{
	monitoring = enable;
}

int Device::getState(void)
{
	return state;
}

string Device::getStateStr(void)
{
	switch (state) {
	case BOOTLOADER:
		return "[Bootloader]";
	case OFFLINE:
		return "[Offline]";
	case ONLINE:
		return "[Online]";
	case READY:
		return "[Ready]";
	case CONNECTING:
		return "[Connecting]";
	case UNKNOWN:
	default:
		return "[Unknown]";
	}
}

void Device::setState(int state)
{
	this->state = state;
}

int Device::parseState(string stateStr)
{
	if (!stateStr.compare("bootloader")) {
		return BOOTLOADER;
	} else if (!stateStr.compare("offline")) {
		return OFFLINE;
	} else if (!stateStr.compare("device")) {
		return ONLINE;
	} else {
		return UNKNOWN;
	}
}

void Device::setOutputWnd(HWND hDlg, HWND hCtrl)
{
	screenDlg  = hDlg;
	screenCtrl = hCtrl;
}

void Device::startScreenshot(void)
{
	if (!show) {
		show = true;
		requestScreenshot();
	}
}

void Device::stopScreenshot(void)
{
	show = false;
}

void Device::onScreenshot(LPVOID image)
{
	if (!show) {
		return;
	}
	showScreenshot(image);
	requestScreenshot();
}
