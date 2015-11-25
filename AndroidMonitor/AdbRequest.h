#pragma once

#include <string>
#include <list>
#include <Windows.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#include "ByteBuffer.h"
#include "AdbDevice.h"
#include "ResponseData.h"

using namespace std;

class AdbRequest
{
public:
	AdbRequest(AdbDevice* dev);
	~AdbRequest(void);

protected:
	char          TAG[32];
	list<string>  _reqqueue; // request queue
	HANDLE        _qNotifier;
	AdbDevice*    _device;
	string        _serialnumber;
	HANDLE        _listener;
	bool          _readTogether;

	bool          postRequest(string req);
	string        readResponseStr(ByteBuffer* buf);
	virtual DWORD maxQueueSize(void) = 0;
	virtual int   priority(void) = 0;
	virtual void  readResponse(ByteBuffer* data) = 0; // read successful response.

private:
	bool                _listen;
	LPADDRINFO          _addrinfo; // The address information of adb server
	CRITICAL_SECTION    _csQueue;

	static DWORD WINAPI listen(LPVOID lpParam);
	void                consume(void);
	bool                sendRequestStr(SOCKET soket, string req);
};
