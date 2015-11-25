#pragma once

#include "NetRequest.h"

class NetScreenshotRequest : public NetRequest
{
public:
	NetScreenshotRequest(NetDevice* dev);
	~NetScreenshotRequest(void);

private:
	int priority(void);
	void readResponse(ByteBuffer* data);
};
