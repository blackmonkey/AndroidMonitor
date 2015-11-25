#include "StdAfx.h"
#include "StrSafe.h"
#include "NetRequest.h"
#include "Utils.h"

NetRequest::NetRequest(NetDevice* dev)
{
	StringCchPrintf(TAG, sizeof(TAG), _T("NetRequest"));

	_device = dev;
	_addrinfo = dev->getAddrinfo();

	_rNotifier = CreateEvent(NULL, true, false, NULL);
	if (!_rNotifier) {
		Utils::err(TAG, "Failed to create request notifier", WSAGetLastError());
		throw exception("Failed to create request notifier!");
	}

	// create request event listener.
	DWORD threadid;
	_listen = true;
	_listener = CreateThread(NULL, 0, NetRequest::request, this, CREATE_SUSPENDED, &threadid);
	if (!_listener) {
		Utils::err(TAG, "Failed to create event queue listener", WSAGetLastError());
		throw exception("Failed to create event queue listener!");
	}
}

NetRequest::~NetRequest(void)
{
	_listen = false;

	if (_rNotifier) {
		ResetEvent(_rNotifier);
		CloseHandle(_rNotifier);
		_rNotifier = NULL;
	}

	_addrinfo = NULL;

	if (_listener) {
		WaitForSingleObject(_listener, 3000);

		DWORD exitCode = 0;
		GetExitCodeThread(_listener, &exitCode);
		if (STILL_ACTIVE == exitCode) {
			TerminateThread(_listener, 0);
		}

		CloseHandle(_listener);
		_listener = NULL;
	}
}

/**
 * Thread to post request and read response.
 * @param lpParam Pointer to NetRequest instance.
 * @return S_OK for success
 */
DWORD WINAPI NetRequest::request(LPVOID lpParam)
{
	NetRequest* obj = (NetRequest*)lpParam;
	WSANETWORKEVENTS netEvents = {0};
	ResponseData respData = {0};
	WSAEVENT stEvent = WSA_INVALID_EVENT;
	SOCKET soket;
	vector<ResponseData> allResp;
	int res = 0, rc = 0, bytesRead = 0, bytesToRead = 0;

	LOG(obj->TAG, "0x%08X request() BEGIN", obj);

	HANDLE hnd = INVALID_HANDLE_VALUE;
	DuplicateHandle(GetCurrentProcess(), obj->_listener,
		GetCurrentProcess(), &hnd, 0,FALSE, DUPLICATE_SAME_ACCESS);
	if(!SetThreadPriority(hnd, obj->priority())) {
		Utils::err(obj->TAG, "listen() failed to set priority", GetLastError());
	}

	// create overlapped socket, defaultly.
	soket = socket(obj->_addrinfo->ai_family, obj->_addrinfo->ai_socktype, obj->_addrinfo->ai_protocol);
	if (soket == INVALID_SOCKET) {
		Utils::err(obj->TAG, "socket() failed", WSAGetLastError());
		throw exception("Failed to create socket in request()!");
	}

	LOG(obj->TAG, "0x%08X request() soket=0x%08X request=%s", obj, soket, obj->_request.c_str());

	if (WSA_INVALID_EVENT == (stEvent = WSACreateEvent())) {
		Utils::err(obj->TAG, "request() failed to create event", WSAGetLastError());
		res = E_FAIL;
		goto request_end;
	}

	LOG(obj->TAG, "0x%08X request() stEvent=0x%08X", obj, stEvent);

	if (SOCKET_ERROR == WSAEventSelect(soket, stEvent, FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE)) {
		Utils::err(obj->TAG, "request() failed to select event", WSAGetLastError());
		res = E_FAIL;
		goto request_end;
	}

	rc = connect(soket,
				 const_cast<sockaddr*>(obj->_addrinfo->ai_addr),
				 sizeof(struct sockaddr));
	if (rc == SOCKET_ERROR) {
		rc = WSAGetLastError();
		if (rc != WSAEWOULDBLOCK && rc != WSAEALREADY && rc != WSAEISCONN) {
			Utils::err(obj->TAG, "request() connect() failed", rc);
			res = E_FAIL;
			goto request_end;
		}
	}

	LOG(obj->TAG, "0x%08X request() _listen=%d", obj, obj->_listen);

//#if LOG_VAL == 1
	int loopCount = 0;
//#endif

	while (obj->_listen) {
//#if LOG_VAL == 1
		loopCount++;
//#endif
		LOG(obj->TAG, "0x%08X request() [%d] wait for 0x%08X", obj, loopCount, stEvent);

		rc = WSAWaitForMultipleEvents(1, &stEvent, FALSE, WSA_INFINITE, FALSE);
		if (rc == WSA_WAIT_FAILED) {
			Utils::err(obj->TAG, "request() failed on wait event", WSAGetLastError());
			res = E_FAIL;
			break;
		}
		LOG(obj->TAG, "0x%08X request() [%d] got 0x%08X", obj, loopCount, stEvent);

		rc = WSAEnumNetworkEvents(soket, stEvent, &netEvents);
		if (rc == SOCKET_ERROR) {
			Utils::err(obj->TAG, "request() failed to enum event", WSAGetLastError());
			res = E_FAIL;
			break;
		}

		/**
		 * It is possible that netEvents.lNetworkEvents contains multiple events.
		 * Handle FD_READ and RD_WRITE first.
		 */
		if (netEvents.lNetworkEvents & FD_READ) {
			LOG(obj->TAG, "0x%08X request() [%d] got 0x%08X FD_READ", obj, loopCount, stEvent);
			if (0 != (rc = netEvents.iErrorCode[FD_READ_BIT])) {
				Utils::err(obj->TAG, "request() failed to read", rc);
				res = E_FAIL;
				break;
			}

			// Try to receive some data.
			respData.size = recv(soket, respData.buf, RESP_DATA_BLOCK_SIZE, 0);
			if ((SOCKET_ERROR == respData.size) && (WSAEWOULDBLOCK != (rc = WSAGetLastError()))) {
				// If WSAEWOULDBLOCK occurs, go through to WSAWaitForMultipleEvents(),
				// Otherwise, break loop here.
				Utils::err(obj->TAG, "request() recv() failed", rc);
				res = E_FAIL;
				break;
			} else if (respData.size == 0) {
				// The connection has been gracefully closed
				LOG(obj->TAG, "0x%08X request() [%d] FD_READ close", obj, loopCount);
				break;
			} else if (respData.size > 0) {
				bytesRead += respData.size;
				allResp.push_back(respData);

				LOG(obj->TAG, "0x%08X request() [%d] FD_READ %d bytes, total %d bytes", obj, loopCount, respData.size, bytesRead);

				if (bytesRead >= 4 && bytesToRead == 0) {
					BYTE* pBytesToRead = (BYTE*)&bytesToRead;
					BYTE* pLimit = pBytesToRead + sizeof(bytesToRead);
					int bytes = 0;
					for (int i = 0; i < allResp.size() && pBytesToRead < pLimit; i++) {
						bytes = Utils::min(pLimit - pBytesToRead, allResp[i].size);
						CopyMemory(pBytesToRead, allResp[i].buf, bytes);
						pBytesToRead += bytes;
					}
				}

				if (bytesToRead > 0 && bytesRead >= bytesToRead) {
					obj->readAllResponse(allResp, bytesRead);
					allResp.clear();
					bytesToRead = 0;
					bytesRead = 0;

					// Wait for next request posting
					if (WAIT_OBJECT_0 != WaitForSingleObject(obj->_rNotifier, INFINITE)) {
						Utils::err(obj->TAG, "listen() failed on waiting for event notifier", GetLastError());
					}
					WSASetEvent(stEvent);
				}
			}
		}
		if ((netEvents.lNetworkEvents & FD_WRITE) || // server indicate that client can write/post request
			(netEvents.lNetworkEvents == 0) // triggered by WSASetEvent(stEvent) to post request once more
			) {
			LOG(obj->TAG, "0x%08X request() [%d] got 0x%08X FD_WRITE", obj, loopCount, stEvent);
			if (0 != (rc = netEvents.iErrorCode[FD_WRITE_BIT])) {
				Utils::err(obj->TAG, "request() failed to write", rc);
				res = E_FAIL;
				break;
			}
			if (SOCKET_ERROR == send(soket, obj->_request.c_str(), obj->_request.length(), 0)) {
				string msg = "send '" + obj->_request + "' failed";
				Utils::err(obj->TAG, msg.c_str(), WSAGetLastError());
				res = E_FAIL;
				break;
			}
		}
		if (netEvents.lNetworkEvents & FD_CONNECT) {
			LOG(obj->TAG, "0x%08X request() [%d] got 0x%08X FD_CONNECT", obj, loopCount, stEvent);
			if (0 != (rc = netEvents.iErrorCode[FD_CONNECT_BIT])) {
				Utils::err(obj->TAG, "request() failed to connect", rc);
				res = E_FAIL;
				break;
			}
		}
		if (netEvents.lNetworkEvents & FD_CLOSE) {
			LOG(obj->TAG, "0x%08X request() [%d] got 0x%08X FD_CLOSE", obj, loopCount, stEvent);
			if (0 != (rc = netEvents.iErrorCode[FD_CLOSE_BIT])) {
				Utils::err(obj->TAG, "request() failed to close", rc);
				// ignore close error, since all response data are already received to here.
				//res = E_FAIL;
			}
			break;
		}
	}

request_end:
	if (soket != INVALID_SOCKET) {
		// In case the daemon thread is blocked for waiting WSA events,
		// send a nudge via the socket to unblock it, and then close
		// socket.
		//send(soket, "\0", 1, 0);
		LOG(obj->TAG, "0x%08X request() shutdown 0x%08X", obj, soket);
		shutdown(soket, SD_BOTH);
		LOG(obj->TAG, "0x%08X request() closesocket 0x%08X", obj, soket);
		closesocket(soket);
	}

	if (stEvent != WSA_INVALID_EVENT) {
		LOG(obj->TAG, "0x%08X request() close event 0x%08X", obj, stEvent);
		WSACloseEvent(stEvent);
	}

	if (SUCCEEDED(res)) {
		obj->readAllResponse(allResp, bytesRead);
		allResp.clear();
	}

	LOG(obj->TAG, "0x%08X request() END", obj);
}

void NetRequest::readAllResponse(vector<ResponseData> resps, int bytes)
{
	if (resps.size() <= 0 || bytes <= 0) {
		return;
	}

	// concatenate response data and handle them
	ByteBuffer* data = new ByteBuffer(bytes);
	for (int i = 0; i < resps.size(); i++) {
		data->append(resps[i].buf, resps[i].size);
		LOG(TAG, "0x%08X request() success, append %d bytes to data", this, resps[i].size);
	}
	readResponse(data);
	delete data;
}

void NetRequest::post(void)
{
	ResumeThread(_listener);
	if (_rNotifier != NULL) {
		SetEvent(_rNotifier);
	}
}
