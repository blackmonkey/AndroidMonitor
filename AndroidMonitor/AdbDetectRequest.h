#pragma once

#include <string>
#include "AdbRequest.h"

using namespace std;

class AdbDetectRequest : public AdbRequest
{
public:
	AdbDetectRequest(AdbDevice* dev);
	~AdbDetectRequest(void);
	bool  post(void);

private:
	DWORD maxQueueSize(void);
	int   priority(void);
	void  readResponse(ByteBuffer* data);
	bool  findKey(string line);
	bool  findAbs(string line, bool findX);
};
