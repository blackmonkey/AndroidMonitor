#pragma once

#include <string>
#include "AdbRequest.h"

using namespace std;

class AdbScreenshotRequest : public AdbRequest
{
public:
	AdbScreenshotRequest(AdbDevice* dev);
	~AdbScreenshotRequest(void);
	bool   post(void);

private:
	DWORD  maxQueueSize(void);
	int    priority(void);
	void   readResponse(ByteBuffer* data);
};
