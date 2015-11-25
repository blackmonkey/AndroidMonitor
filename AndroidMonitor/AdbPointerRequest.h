#pragma once

#include <string>
#include "AdbRequest.h"

using namespace std;

class AdbPointerRequest : public AdbRequest
{
public:
	AdbPointerRequest(AdbDevice* dev, string pointerDev);
	~AdbPointerRequest(void);
	bool down(int x, int y);
	bool move(int x, int y);
	bool up(void);

private:
	string pointerDownRequestFormat;
	string pointerMoveRequestFormat;
	string pointerUpRequest;

	DWORD  maxQueueSize(void);
	int    priority(void);
	void   readResponse(ByteBuffer* data);
};
