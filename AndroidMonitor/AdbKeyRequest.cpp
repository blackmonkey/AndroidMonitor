#include "StdAfx.h"
#include "AdbKeyRequest.h"
#include "strutil.h"
#include "Utils.h"
#include <StrSafe.h>

AdbKeyRequest::AdbKeyRequest(AdbDevice* dev) : AdbRequest(dev)
{
	StringCchPrintf(TAG, sizeof(TAG), _T("AdbKeyRequest"));

	LOG(TAG, "0x%08X AdbKeyRequest() dev=0x%08X, sn='%s', listen thread=0x%08X", this, _device, _serialnumber.c_str(), _listener);
	ResumeThread(_listener);
}

AdbKeyRequest::~AdbKeyRequest(void)
{
	LOG(TAG, "0x%08X ~AdbKeyRequest() dev=0x%08X, sn='%s', listen thread=0x%08X", this, _device, _serialnumber.c_str(), _listener);
}

bool AdbKeyRequest::strike(int keycode)
{
	return postRequest("shell:input keyevent " + strutil::toString(keycode));
}

DWORD AdbKeyRequest::maxQueueSize(void)
{
	return 500;
}

int AdbKeyRequest::priority(void)
{
	return THREAD_PRIORITY_ABOVE_NORMAL;
}

void AdbKeyRequest::readResponse(ByteBuffer* data)
{
	// ignore the response
}
