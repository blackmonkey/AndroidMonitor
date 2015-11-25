#include "StdAfx.h"
#include "strutil.h"
#include "AdbRequest.h"
#include "Utils.h"
#include <Mswsock.h>
#include <vector>
#include <StrSafe.h>

AdbRequest::AdbRequest(AdbDevice* dev)
{
	StringCchPrintf(TAG, sizeof(TAG), _T("AdbRequest"));

	// When dev is NULL, the request doesn't need to switch transport
	_device = dev;
	if (dev) {
		_serialnumber = dev->getName();
	}

	_readTogether = true;

	InitializeCriticalSection(&_csQueue);

	EnterCriticalSection(&_csQueue);
	_reqqueue.clear();
	LeaveCriticalSection(&_csQueue);

	_qNotifier = CreateEvent(NULL, true, false, NULL);
	if (!_qNotifier) {
		Utils::err(TAG, "Failed to create event queue notifier", WSAGetLastError());
		throw exception("Failed to create event queue notifier!");
	}

	_addrinfo = dev->getAddrinfo();

	// create request event listener.
	DWORD threadid;
	_listen = true;
	_listener = CreateThread(NULL, 0, AdbRequest::listen, this, CREATE_SUSPENDED, &threadid);
	if (!_listener) {
		Utils::err(TAG, "Failed to create event queue listener", WSAGetLastError());
		throw exception("Failed to create event queue listener!");
	}

	LOG(TAG, "0x%08X AdbRequest() dev=0x%08X, sn='%s', listen thread=0x%08X", this, _device, (_serialnumber.length() > 0) ? _serialnumber.c_str() : "", _listener);
}

AdbRequest::~AdbRequest(void)
{
	LOG(TAG, "0x%08X ~AdbRequest() dev=0x%08X, sn='%s', listen thread=0x%08X", this, _device, (_serialnumber.length() > 0) ? _serialnumber.c_str() : "", _listener);

	EnterCriticalSection(&_csQueue);
	_reqqueue.clear();
	LeaveCriticalSection(&_csQueue);

	_listen = false;

	if (_qNotifier) {
		ResetEvent(_qNotifier);
		CloseHandle(_qNotifier);
		_qNotifier = NULL;
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

	DeleteCriticalSection(&_csQueue);
}

/**
 * Request event listener thread.
 * @param lpParam Pointer to AdbRequest instance.
 * @return S_OK for success
 */
DWORD WINAPI AdbRequest::listen(LPVOID lpParam)
{
	AdbRequest* obj = (AdbRequest*)lpParam;

	LOG(obj->TAG, "0x%08X listen() BEGIN", lpParam);

	HANDLE hnd = INVALID_HANDLE_VALUE;
	DuplicateHandle(GetCurrentProcess(), obj->_listener,
		GetCurrentProcess(), &hnd, 0,FALSE, DUPLICATE_SAME_ACCESS);
	if(!SetThreadPriority(hnd, obj->priority())) {
		Utils::err(obj->TAG, "listen() failed to set priority", GetLastError());
	}

	LOG(obj->TAG, "0x%08X listen() _listen=%d", lpParam, obj->_listen);

	while (obj->_listen) {
		EnterCriticalSection(&obj->_csQueue);

		LOG(obj->TAG, "0x%08X listen() _reqqueue.empty()=%d", lpParam, obj->_reqqueue.empty());

		if (obj->_reqqueue.empty()) {
			// set _qNotifier to nonsignaled to wait for new event.
			ResetEvent(obj->_qNotifier);
			LOG(obj->TAG, "0x%08X listen() ResetEvent(0x%08X)", lpParam, obj->_qNotifier);
		}
		LeaveCriticalSection(&obj->_csQueue);

		LOG(obj->TAG, "0x%08X listen() wait for 0x%08X", lpParam, obj->_qNotifier);
		if (WAIT_OBJECT_0 != WaitForSingleObject(obj->_qNotifier, INFINITE)) {
			Utils::err(obj->TAG, "listen() failed on waiting for event notifier", GetLastError());
		}
		LOG(obj->TAG, "0x%08X listen() got 0x%08X", lpParam, obj->_qNotifier);

		// consume last event
		obj->consume();
	}

	LOG(obj->TAG, "0x%08X listen() END", lpParam);

	return S_OK;
}

/**
 * Consume last request in the event queue
 */
void AdbRequest::consume(void)
{
	LOG(TAG, "0x%08X consume() BEGIN", this);

	WSANETWORKEVENTS netEvents = {0};
	ResponseData respData = {0};
	WSAEVENT stEvent = WSA_INVALID_EVENT;
	SOCKET soket;
	string request;
	vector<ResponseData> allResp;
	int res = 0, rc = 0, bytesRead = 0;

	// true to switch transport to device, false to send concrete request
	bool setdevice = (/*_serialnumber != NULL &&*/ _serialnumber.length() > 0);

	// create overlapped socket, defaultly.
	soket = socket(_addrinfo->ai_family, _addrinfo->ai_socktype, _addrinfo->ai_protocol);
	if (soket == INVALID_SOCKET) {
		Utils::err(TAG, "socket() failed", WSAGetLastError());
		throw exception("Failed to create socket in consume()!");
	}

	LOG(TAG, "0x%08X consume() soket=0x%08X", this, soket);

	EnterCriticalSection(&_csQueue);
	request = _reqqueue.front();
	_reqqueue.pop_front();
	LeaveCriticalSection(&_csQueue);

	LOG(TAG, "0x%08X consume() request=%s", this, request.c_str());

	if (WSA_INVALID_EVENT == (stEvent = WSACreateEvent())) {
		Utils::err(TAG, "consume() failed to create event", WSAGetLastError());
		res = E_FAIL;
		goto consume_end;
	}

	LOG(TAG, "0x%08X consume() stEvent=0x%08X", this, stEvent);

	if (SOCKET_ERROR == WSAEventSelect(soket, stEvent, FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE)) {
		Utils::err(TAG, "consume() failed to select event", WSAGetLastError());
		res = E_FAIL;
		goto consume_end;
	}

	rc = connect(soket,
				 const_cast<sockaddr*>(_addrinfo->ai_addr),
				 sizeof(struct sockaddr));
	if (rc == SOCKET_ERROR) {
		rc = WSAGetLastError();
		if (rc != WSAEWOULDBLOCK && rc != WSAEALREADY && rc != WSAEISCONN) {
			Utils::err(TAG, "consume() connect() failed", rc);
			res = E_FAIL;
			goto consume_end;
		}
	}

	LOG(TAG, "0x%08X consume() _listen=%d", this, _listen);

#if LOG_VAL == 1
	int loopCount = 0;
#endif

	while (_listen) {
#if LOG_VAL == 1
		loopCount++;
#endif
		LOG(TAG, "0x%08X consume() [%d] wait for 0x%08X", this, loopCount, stEvent);

		rc = WSAWaitForMultipleEvents(1, &stEvent, FALSE, WSA_INFINITE, FALSE);
		if (rc == WSA_WAIT_FAILED) {
			Utils::err(TAG, "consume() failed on wait event", WSAGetLastError());
			res = E_FAIL;
			break;
		}
		LOG(TAG, "0x%08X consume() [%d] got 0x%08X", this, loopCount, stEvent);

		rc = WSAEnumNetworkEvents(soket, stEvent, &netEvents);
		if (rc == SOCKET_ERROR) {
			Utils::err(TAG, "consume() failed to enum event", WSAGetLastError());
			res = E_FAIL;
			break;
		}

		/**
		 * It is possible that netEvents.lNetworkEvents contains multiple events.
		 * Handle FD_READ and RD_WRITE first.
		 */
		if (netEvents.lNetworkEvents & FD_READ) {
			LOG(TAG, "0x%08X consume() [%d] got 0x%08X FD_READ", this, loopCount, stEvent);
			if (0 != (rc = netEvents.iErrorCode[FD_READ_BIT])) {
				Utils::err(TAG, "consume() failed to read", rc);
				res = E_FAIL;
				break;
			}

			// Try to receive some data.
			respData.size = recv(soket, respData.buf, RESP_DATA_BLOCK_SIZE, 0);
			if ((SOCKET_ERROR == respData.size) && (WSAEWOULDBLOCK != (rc = WSAGetLastError()))) {
				// If WSAEWOULDBLOCK occurs, go through to WSAWaitForMultipleEvents(),
				// Otherwise, break loop here.
				Utils::err(TAG, "consume() recv() failed", rc);
				res = E_FAIL;
				break;
			} else if (respData.size == 0) {
				// The connection has been gracefully closed
				LOG(TAG, "0x%08X consume() [%d] FD_READ close", this, loopCount);
				break;
			} else if (respData.size > 0) {
				if (_readTogether) {
					bytesRead += respData.size;
					LOG(TAG, "0x%08X consume() [%d] FD_READ %d bytes, total %d bytes", this, loopCount, respData.size, bytesRead);
					allResp.push_back(respData);
					LOG(TAG, "0x%08X consume() [%d] FD_READ setdevice=%d", this, loopCount, setdevice);
					if (setdevice && bytesRead >= 4) {
						char status[8] = {0};
						int num = 0;
						for (int i = 0; i < allResp.size() && num < sizeof(status); i++) {
							for (int j = 0; j < allResp[i].size && num < sizeof(status); j++) {
								status[num++] = allResp[i].buf[j];
								LOG(TAG, "0x%08X consume() [%d] FD_READ status[%d]='%c'", this, loopCount, num - 1, status[num - 1]);
							}
						}
						LOG(TAG, "consume() status='%s'", status);
						if (Utils::isOkay(status)) {
							setdevice = false;
							allResp.clear();
							bytesRead = 0;
							LOG(TAG, "0x%08X consume() [%d] FD_READ send request", this, loopCount);
							if (!sendRequestStr(soket, request)) {
								res = E_FAIL;
								break;
							}
						}
					}
				} else {
					ByteBuffer* data = new ByteBuffer(respData.size);
					data->append(respData.buf, respData.size);
					data->reset();
					readResponse(data);
					delete data;
				}
			}
		}
		if (netEvents.lNetworkEvents & FD_WRITE) {
			LOG(TAG, "0x%08X consume() [%d] got 0x%08X FD_WRITE", this, loopCount, stEvent);
			if (0 != (rc = netEvents.iErrorCode[FD_WRITE_BIT])) {
				Utils::err(TAG, "consume() failed to write", rc);
				res = E_FAIL;
				break;
			}
			if (!sendRequestStr(soket, setdevice ? "host:transport:" + _serialnumber : request)) {
				res = E_FAIL;
				break;
			}
		}
		if (netEvents.lNetworkEvents & FD_CONNECT) {
			LOG(TAG, "0x%08X consume() [%d] got 0x%08X FD_CONNECT", this, loopCount, stEvent);
			if (0 != (rc = netEvents.iErrorCode[FD_CONNECT_BIT])) {
				Utils::err(TAG, "consume() failed to connect", rc);
				res = E_FAIL;
				break;
			}
		}
		if (netEvents.lNetworkEvents & FD_CLOSE) {
			LOG(TAG, "0x%08X consume() [%d] got 0x%08X FD_CLOSE", this, loopCount, stEvent);
			if (0 != (rc = netEvents.iErrorCode[FD_CLOSE_BIT])) {
				Utils::err(TAG, "consume() failed to close", rc);
				// ignore close error, since all response data are already received to here.
				//res = E_FAIL;
			}
			break;
		}
	}

consume_end:
	if (soket != INVALID_SOCKET) {
		// In case the daemon thread is blocked for waiting WSA events,
		// send a nudge via the socket to unblock it, and then close
		// socket.
		//send(soket, "\0", 1, 0);
		LOG(TAG, "0x%08X consume() shutdown 0x%08X", this, soket);
		shutdown(soket, SD_BOTH);
		LOG(TAG, "0x%08X consume() closesocket 0x%08X", this, soket);
		closesocket(soket);
	}

	if (stEvent != WSA_INVALID_EVENT) {
		LOG(TAG, "0x%08X consume() close event 0x%08X", this, stEvent);
		WSACloseEvent(stEvent);
	}

	if (FAILED(res)) {
		LOG(TAG, "0x%08X consume() failed, bytesRead=%d, push request '%s' back", this, bytesRead, request.c_str());

		// If failed to handle whole request event, push it back to event queue,
		// so that it can be handled again in the next consume loop.
		EnterCriticalSection(&_csQueue);
		_reqqueue.push_front(request);
		LeaveCriticalSection(&_csQueue);

		LOG(TAG, "0x%08X consume() failed, notify event 0x%08X", this, _qNotifier);
		SetEvent(_qNotifier);
	} else if (bytesRead > 0) {
		LOG(TAG, "0x%08X consume() success, bytesRead=%d", this, bytesRead);

		// concatenate response data and handle them
		ByteBuffer* data = new ByteBuffer(bytesRead);
		for (int i = 0; i < allResp.size(); i++) {
			data->append(allResp[i].buf, allResp[i].size);
			LOG(TAG, "0x%08X consume() success, append %d bytes to data", this, allResp[i].size);
		}
		data->reset();
		if (data->size() >= 4) {
			if (Utils::isOkay(data->buf())) {
				data->seek(4);
				readResponse(data);
			} else if (Utils::isFail(data->buf())) {
				data->seek(4);
				string msg = readResponseStr(data);
				LOG(TAG, "Get FAIL response:%s", msg.c_str());
			}
		} else {
			readResponse(data);
		}
		delete data;
	}

	allResp.clear();

	LOG(TAG, "0x%08X consume() END", this);
}

bool AdbRequest::sendRequestStr(SOCKET soket, string req)
{
	string data = strutil::toHexString(req.length(), 4) + req;

	LOG(TAG, "0x%08X sendRequestStr() soket=0x%08X request='%s'", this, soket, data.c_str());

	if (SOCKET_ERROR == send(soket, data.c_str(), data.length(), 0)) {
		string msg = "sendRequestStr(" + req + ") failed";
		Utils::err(TAG, msg.c_str(), WSAGetLastError());
		return false;
	}
	return true;
}

bool AdbRequest::postRequest(string req)
{
	bool res = false;

	LOG(TAG, "0x%08X postRequest(%s)", this, req.c_str());

	EnterCriticalSection(&_csQueue);
	if (_reqqueue.size() < maxQueueSize()) {
		_reqqueue.push_back(req);
		SetEvent(_qNotifier);
		res = true;
	}
	LeaveCriticalSection(&_csQueue);

	return res;
}

string AdbRequest::readResponseStr(ByteBuffer* buf)
{
	// read size in bytes from buffer.
	string len = buf->getStr(4);
	int bytes = strtol(len.c_str(), NULL, 16);
	if (bytes <= 0) {
		LOG(TAG, "readResponseStr():No response! length = %d!", bytes);
		return "";
	}

	return buf->getStr(bytes);
}
