#pragma once

#include <string>
#include "AdbRequest.h"

using namespace std;

class AdbKeyRequest : public AdbRequest
{
public:
	AdbKeyRequest(AdbDevice* dev);
	~AdbKeyRequest(void);
	bool strike(int keycode);

private:
	DWORD  maxQueueSize(void);
	int    priority(void);
	void   readResponse(ByteBuffer* data);
};
