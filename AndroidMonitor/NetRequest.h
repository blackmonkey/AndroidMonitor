#pragma once

#include "NetDevice.h"
#include "ResponseData.h"
#include "ByteBuffer.h"
#include <vector>
#include <Windows.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>

using namespace std;

class NetRequest
{
public:
	NetRequest(NetDevice* dev);
	~NetRequest(void);
	void post(void);

protected:
	char TAG[32];
	string _request;
	NetDevice* _device;

	virtual int priority(void) = 0;
	virtual void readResponse(ByteBuffer* data) = 0; // read successful response.

private:
	LPADDRINFO _addrinfo; // The address information of adb server
	HANDLE _rNotifier;
	HANDLE _listener;
	bool _listen;

	static DWORD WINAPI request(LPVOID lpParam);
	void readAllResponse(vector<ResponseData> resps, int bytes);
};
