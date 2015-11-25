#include "StdAfx.h"
#include "AdbScreenshotRequest.h"
#include "Utils.h"
#include "RawImage.h"
#include <StrSafe.h>

AdbScreenshotRequest::AdbScreenshotRequest(AdbDevice* dev) : AdbRequest(dev)
{
	StringCchPrintf(TAG, sizeof(TAG), _T("AdbScreenshotRequest"));

	LOG(TAG, "0x%08X AdbScreenshotRequest() dev=0x%08X, sn='%s', listen thread=0x%08X", this, _device, _serialnumber.c_str(), _listener);
	ResumeThread(_listener);
}

AdbScreenshotRequest::~AdbScreenshotRequest(void)
{
	LOG(TAG, "0x%08X ~AdbScreenshotRequest() dev=0x%08X, sn='%s', listen thread=0x%08X", this, _device, _serialnumber.c_str(), _listener);
}

/**
 * Post a request to get a screenshot and show it.
 * @return Whether the request is successfully posted.
 */
bool AdbScreenshotRequest::post(void)
{
	return postRequest("framebuffer:");
}

DWORD AdbScreenshotRequest::maxQueueSize(void)
{
	return 500;
}

int AdbScreenshotRequest::priority(void)
{
	return THREAD_PRIORITY_LOWEST;
}

void AdbScreenshotRequest::readResponse(ByteBuffer* data)
{
	// get version and header size of screenshot data
	int version = data->getInt();
	int headerSize = RawImage::getHeaderSize(version);

	LOG(TAG, "0x%08X readResponse() dev=0x%08X, sn='%s', ver=%d, headersize=%d", this, _device, _serialnumber.c_str(), version, headerSize);

#ifdef _DEBUG
	SYSTEMTIME lt;
	GetLocalTime(&lt);

	char shotFile[MAX_PATH] = {0};
	int len = GetModuleFileName(NULL, shotFile, MAX_PATH);
	while (shotFile[--len] != '\\') {}

	StringCchPrintfA(&shotFile[++len], MAX_PATH - len, "%s_screenshot_%04d%02d%02d%02d%02d%02d.dat",
		_device->getName().c_str(), lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);
#endif

	if (headerSize <= 0) {
		Utils::err(TAG, "Invalid/Unsupported screenshot data header!");
#ifdef _DEBUG
		data->save(shotFile);
#endif
		return;
	}

	// to here, it is confirmed that the version of screenshot data is supported.
	RawImage* image = new RawImage();
	if (!image->readHeader(version, *data) || image->size() <= 0) {
		delete image;
		Utils::err(TAG, "Invalid/Unsupported screenshot data header. version: %d, image size: %d", version, image->size());
#ifdef _DEBUG
		data->save(shotFile);
#endif
		return;
	}

	// to here, it is confirmed that the header of screenshot data is valid
	CopyMemory(image->data(),
			   data->buf() + data->pos(),
			   Utils::min(data->size() - data->pos(), image->size()));
	_device->onScreenshot(image);
}
