#include "StdAfx.h"
#include "NetScreenshotRequest.h"
#include "Utils.h"
#include "FreeImage.h"
#include <StrSafe.h>

NetScreenshotRequest::NetScreenshotRequest(NetDevice* dev) : NetRequest(dev)
{
	StringCchPrintf(TAG, sizeof(TAG), _T("NetScreenshotRequest"));
	_request = "screenshot;";
}

NetScreenshotRequest::~NetScreenshotRequest(void)
{
}

int NetScreenshotRequest::priority(void)
{
	return THREAD_PRIORITY_LOWEST;
}

void NetScreenshotRequest::readResponse(ByteBuffer* data)
{
	// The first 4 bytes in data is bytesToRead
	// The second 4 bytes in data is "OKAY" or "FAIL"
	if (data->size() <= 8) {
		LOG(TAG, "Get no response");
		return;
	}
	if (Utils::isFail(data->buf() + 4)) {
		LOG(TAG, "Get FAIL response");
		return;
	}

	// To here, the response is "OKAY" followed by screenshot information
	data->reset();
	data->seek(8);

	int format = data->getInt();
	int width = data->getInt();
	int height = data->getInt();

	// attach the binary data to a memory stream
	FIMEMORY *fiMem = FreeImage_OpenMemory((BYTE*)(data->buf() + data->pos()), data->size() - data->pos());
	// get the file type
	FREE_IMAGE_FORMAT fiFormat = FreeImage_GetFileTypeFromMemory(fiMem, 0);
	// load an image from the memory stream
	FIBITMAP *fiBmp = FreeImage_LoadFromMemory(fiFormat, fiMem, 0);
	// show bitmap
	_device->onScreenshot(fiBmp);
	FreeImage_Unload(fiBmp);
	// always close the memory stream
	FreeImage_CloseMemory(fiMem);
}
