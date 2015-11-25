#include "StdAfx.h"
#include "AdbPointerRequest.h"
#include "strutil.h"
#include "Utils.h"
#include <StrSafe.h>

AdbPointerRequest::AdbPointerRequest(AdbDevice* dev, string pointerDev) : AdbRequest(dev)
{
	StringCchPrintf(TAG, sizeof(TAG), _T("AdbPointerRequest"));

	pointerDownRequestFormat  = "shell:";
	pointerDownRequestFormat += "sendevent " + pointerDev + " 3 0 %x;";
	pointerDownRequestFormat += "sendevent " + pointerDev + " 3 1 %y;";
	pointerDownRequestFormat += "sendevent " + pointerDev + " 1 330 1;";
	pointerDownRequestFormat += "sendevent " + pointerDev + " 0 0 0";

	pointerMoveRequestFormat  = "shell:";
	pointerMoveRequestFormat += "sendevent " + pointerDev + " 3 0 %x;";
	pointerMoveRequestFormat += "sendevent " + pointerDev + " 3 1 %y;";
	pointerMoveRequestFormat += "sendevent " + pointerDev + " 0 0 0";

	pointerUpRequest  = "shell:";
	pointerUpRequest += "sendevent " + pointerDev + " 1 330 0;";
	pointerUpRequest += "sendevent " + pointerDev + " 0 0 0";

	LOG(TAG, "0x%08X AdbPointerRequest() dev=0x%08X, sn='%s', listen thread=0x%08X", this, _device, _serialnumber.c_str(), _listener);

	ResumeThread(_listener);
}

AdbPointerRequest::~AdbPointerRequest(void)
{
	LOG(TAG, "0x%08X ~AdbPointerRequest() dev=0x%08X, sn='%s', listen thread=0x%08X", this, _device, _serialnumber.c_str(), _listener);
}

bool AdbPointerRequest::down(int x, int y)
{
	string req = pointerDownRequestFormat;
	size_t pos = req.find("%x");
	if (pos == string::npos) {
		Utils::err(TAG, "Failed to encode x position!");
		return false;
	}
	req.replace(pos, 2, strutil::toString(x));

	pos = req.find("%y");
	if (pos == string::npos) {
		Utils::err(TAG, "Failed to encode y position!");
		return false;
	}
	req.replace(pos, 2, strutil::toString(y));
	return postRequest(req);
}

bool AdbPointerRequest::move(int x, int y)
{
	string req = pointerMoveRequestFormat;
	size_t pos = req.find("%x");
	if (pos == string::npos) {
		Utils::err(TAG, "Failed to encode x position!");
		return false;
	}
	req.replace(pos, 2, strutil::toString(x));

	pos = req.find("%y");
	if (pos == string::npos) {
		Utils::err(TAG, "Failed to encode y position!");
		return false;
	}
	req.replace(pos, 2, strutil::toString(y));
	return postRequest(req);
}

bool AdbPointerRequest::up(void)
{
	return postRequest(pointerUpRequest);
}

DWORD AdbPointerRequest::maxQueueSize(void)
{
	return 1000;
}

int AdbPointerRequest::priority(void)
{
	return THREAD_PRIORITY_HIGHEST;
}

void AdbPointerRequest::readResponse(ByteBuffer* data)
{
	// ignore the response
}
