#include "StdAfx.h"
#include "StrSafe.h"
#include "AndroidMonitor.h"
#include "AdbDevice.h"
#include "AdbDetectRequest.h"
#include "AdbKeyRequest.h"
#include "AdbPointerRequest.h"
#include "AdbScreenshotRequest.h"
#include "Utils.h"
#include "RawImage.h"

const char AdbDevice::TAG[12] = "AdbDevice";

AdbDevice::AdbDevice(string sn, string stateStr)
{
	initAddrinfo();

	setState(parseState(stateStr));

	name = sn;

	detectRequest = new AdbDetectRequest(this);
	detectRequest->post();

	keyRequest = new AdbKeyRequest(this);

	pointerRequest = NULL;

	screenshotRequest = new AdbScreenshotRequest(this);

	LOG(TAG, "'%s' created.", sn.c_str());
}

AdbDevice::~AdbDevice(void)
{
	if (detectRequest) {
		delete detectRequest;
		detectRequest = NULL;
	}

	if (keyRequest) {
		delete keyRequest;
		keyRequest = NULL;
	}

	if (pointerRequest) {
		delete pointerRequest;
		pointerRequest = NULL;
	}

	if (screenshotRequest) {
		delete screenshotRequest;
		screenshotRequest = NULL;
	}
}

int AdbDevice::getType(void)
{
	return Device::ADB_DEVICE;
}

string AdbDevice::getName()
{
	return name;
}

string AdbDevice::getDescription()
{
	return name + getStateStr();
}

void AdbDevice::setPointerDeivce(string dev)
{
	LOG(TAG, "setPointerDeivce(%s)", dev.c_str());
	if (pointerRequest) {
		delete pointerRequest;
	}
	pointerRequest = new AdbPointerRequest(this, dev);
}

void AdbDevice::pointerDown(int x, int y)
{
	Utils::log(TAG, "pointerDown(%d, %d)", x, y);
	if (pointerRequest) pointerRequest->down(x, y);
}

void AdbDevice::pointerMove(int x, int y)
{
	LOG(TAG, "pointerMove(%d, %d)", x, y);
	if (pointerRequest) pointerRequest->move(x, y);
}

void AdbDevice::pointerUp(void)
{
	Utils::log(TAG, "pointerUp()");
	if (pointerRequest) pointerRequest->up();
}

void AdbDevice::keyStrike(int keycode)
{
	Utils::log(TAG, "keyPress(%d)", keycode);
	if (keyRequest) keyRequest->strike(keycode);
}

void AdbDevice::requestScreenshot(void)
{
	LOG(TAG, "screenshotRequest=0x%08X, device=0x%08X, sn='%s'", screenshotRequest, this, name.c_str());
	if (screenshotRequest) {
		// Push another screenshot request after a nap
		// Take a nap to avoid refresh screen shot too fast, so that adb server can
		// response key/pointer event in time.
		LOG(TAG, "Sleep(500), device=0x%08X, sn='%s'", this, name.c_str());
		Sleep(500);
		LOG(TAG, "post screenshotRequest, device=0x%08X, sn='%s'", this, name.c_str());
		screenshotRequest->post();
	}
}

void AdbDevice::showScreenshot(LPVOID image)
{
	LOG(TAG, "showScreenshot(0x%08X), device=0x%08X, sn='%s'", image, this, name.c_str());
	if (image) {
		HDC hdc = GetDC(screenCtrl);
		HBITMAP bitmap = ((RawImage*)image)->getBitmap(hdc);
		if (bitmap) {
			SendMessage(screenDlg,
						WM_SHOW_SCREENSHOT,
						MAKEWPARAM(((RawImage*)image)->width(), ((RawImage*)image)->height()),
						(LPARAM)bitmap);
		}
	}
}

void AdbDevice::initAddrinfo(void)
{
	LOG(TAG, "AdbDevice::initAddrinfo().");

	// Initialize the hints to retrieve the server address for IPv4
	ADDRINFO hints    = {0};
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	addinfo = NULL;
	int rc = getaddrinfo("localhost", "5037", &hints, &addinfo);
	if (rc != S_OK) {
		Utils::err(TAG, "getaddrinfo() failed", rc);
		throw exception("getaddrinfo() failed in DeviceRequest()!");
	}
}
