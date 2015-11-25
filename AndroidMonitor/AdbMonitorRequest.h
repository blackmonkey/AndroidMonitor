#pragma once

#include "AdbRequest.h"

class AdbMonitorRequest : public AdbRequest
{
public:
	AdbMonitorRequest(void);
	~AdbMonitorRequest(void);
	bool post(void);

private:
	DWORD maxQueueSize(void);
	int   priority(void);
	void  readResponse(ByteBuffer* data);
};
