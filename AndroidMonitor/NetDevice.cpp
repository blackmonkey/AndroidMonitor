#include "StdAfx.h"
#include "StrSafe.h"
#include "AndroidMonitor.h"
#include "NetDevice.h"
#include "NetScreenshotRequest.h"
#include "Utils.h"
#include "FreeImage.h"
#include <CommCtrl.h>

const char NetDevice::TAG[12] = "NetDevice";

NetDevice::NetDevice(DWORD ip, int port)
{
	setState(READY);

	char ipStr[16] = {0};
	char portStr[8] = {0};

	wsprintf(ipStr, "%d.%d.%d.%d", FIRST_IPADDRESS(ip), SECOND_IPADDRESS(ip), THIRD_IPADDRESS(ip), FOURTH_IPADDRESS(ip));
	_itoa(port, portStr, 10);

	this->ip.assign(ipStr);
	this->port.assign(portStr);

	initAddrinfo();

	screenshotRequest = new NetScreenshotRequest(this);

	Utils::log(TAG, "NetDevice().");
}

NetDevice::~NetDevice(void)
{
	if (screenshotRequest) {
		delete screenshotRequest;
		screenshotRequest = NULL;
	}
}

int NetDevice::getType(void)
{
	return Device::NET_DEVICE;
}

string NetDevice::getName(void)
{
	return ip + ":" + port;
}

string NetDevice::getDescription(void)
{
	return getName() + getStateStr();
}

void NetDevice::pointerDown(int x, int y)
{

}

void NetDevice::pointerMove(int x, int y)
{

}

void NetDevice::pointerUp(void)
{

}

void NetDevice::keyStrike(int keycode)
{

}

void NetDevice::requestScreenshot(void)
{
	if (screenshotRequest) {
		screenshotRequest->post();
	}
}

void NetDevice::showScreenshot(LPVOID image)
{
	LOG(TAG, "showScreenshot(0x%08X), device=0x%08X", image, this);
	if (image) {
		FIBITMAP *fiBmp = (FIBITMAP*)image;
		WORD width = FreeImage_GetWidth(fiBmp);
		WORD height = FreeImage_GetHeight(fiBmp);

		HBITMAP bitmap = CreateDIBitmap(GetDC(screenCtrl),
							FreeImage_GetInfoHeader(fiBmp),
							CBM_INIT,
							FreeImage_GetBits(fiBmp),
							FreeImage_GetInfo(fiBmp),
							DIB_RGB_COLORS);
		if (bitmap) {
/*
			Utils::saveBitmap(FreeImage_GetInfo(fiBmp),
				(char*)FreeImage_GetBits(fiBmp),
				FreeImage_GetDIBSize(fiBmp) - sizeof(BITMAPINFO));
*/
			SendMessage(screenDlg,
				WM_SHOW_SCREENSHOT,
				MAKEWPARAM(width, height),
				(LPARAM)bitmap);
		}
	}
}

void NetDevice::initAddrinfo(void)
{
	Utils::log(TAG, "initAddrinfo().");

	// Initialize the hints to retrieve the server address for IPv4
	ADDRINFO hints    = {0};
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	addinfo = NULL;
	int rc = getaddrinfo(ip.c_str(), port.c_str(), &hints, &addinfo);
	if (rc != S_OK) {
		Utils::err(TAG, "getaddrinfo() failed", rc);
		throw exception("getaddrinfo() failed in DeviceRequest()!");
	}
}
