#include "StdAfx.h"
#include "AdbDetectRequest.h"
#include "strutil.h"
#include "Utils.h"
#include <StrSafe.h>

AdbDetectRequest::AdbDetectRequest(AdbDevice* dev) : AdbRequest(dev)
{
	StringCchPrintf(TAG, sizeof(TAG), _T("AdbDetectRequest"));

	LOG(TAG, "0x%08X AdbDetectRequest() dev=0x%08X, sn='%s', listen thread=0x%08X", this, _device, _serialnumber.c_str(), _listener);
	ResumeThread(_listener);
}

AdbDetectRequest::~AdbDetectRequest(void)
{
	LOG(TAG, "0x%08X ~AdbDetectRequest() dev=0x%08X, sn='%s', listen thread=0x%08X", this, _device, _serialnumber.c_str(), _listener);
}

/**
 * Push a request to detect pointer device.
 * @return Whether the request is successfully posted.
 */
bool AdbDetectRequest::post(void)
{
	return postRequest("shell:getevent -p");
}

DWORD AdbDetectRequest::maxQueueSize(void)
{
	return 1;
}

int AdbDetectRequest::priority(void)
{
	return THREAD_PRIORITY_LOWEST;
}

void AdbDetectRequest::readResponse(ByteBuffer* data)
{
	LOG(TAG, "0x%08X readResponse(0x%08X) BEGIN", this, data);

	bool inKEY = false, inABS = false;
	bool foundKey = false, foundAbsX = false, foundAbsY = false;
	vector<string> pattern;
	vector<string> line;
	vector<string>::iterator it;
	string l, possibleEvents, dev;

	possibleEvents.assign(data->buf() + data->pos(), data->size());
	line = strutil::split(possibleEvents, "\r\n");
	for (it = line.begin(); it < line.end(); it++) {
		if ((*it)[0] != ' ') {
			// new deivce information block, reset all variables
			inKEY = inABS = foundKey = foundAbsX = foundAbsY = false;
			dev = "";
			if (strutil::startsWith((*it), "add device")) {
				pattern = strutil::split((*it), " ");
				dev = pattern.back();
			}
		} else {
			if (strutil::startsWith((*it), "    KEY (0001):")) {
				inKEY = true;
				inABS = false;
				if (!foundKey && findKey((*it).substr(16))) {
					foundKey = true;
				}
			} else if (strutil::startsWith((*it), "    ABS (0003):")) {
				inABS = true;
				inKEY = false;
				l = (*it).substr(16);
				if (!foundAbsX && findAbs(l, true)) {
					foundAbsX = true;
				} else if (!foundAbsY && findAbs(l, false)) {
					foundAbsY = true;
				}
			} else if (strutil::startsWith((*it), "                ")) {
				l = (*it).substr(16);
				if (inKEY && !foundKey && findKey(l)) {
					foundKey = true;
				} else if (inABS) {
					if (!foundAbsX && findAbs(l, true)) {
						foundAbsX = true;
					} else if (!foundAbsY && findAbs(l, false)) {
						foundAbsY = true;
					}
				}
			} else {
				inKEY = inABS = false;
			}
		}

		if (foundKey && foundAbsX && foundAbsY) {
			_device->setPointerDeivce(dev);
			LOG(TAG, "0x%08X readResponse() END pointer device='%s'", this, dev.c_str());
			return;
		}
	}

	_device->setPointerDeivce("");

	LOG(TAG, "0x%08X readResponse() END", this);
}

bool AdbDetectRequest::findKey(string line)
{
	vector<string> pattern = strutil::split(line, " ");
	vector<string>::iterator it;
	for (it = pattern.begin(); it < pattern.end(); it++) {
		if (!(*it).substr(0, 4).compare("014a")) {
			return true;
		}
	}
	return false;
}

bool AdbDetectRequest::findAbs(string line, bool findX)
{
	vector<string> pattern = strutil::split(line, " ");
	string code = (findX) ? "0000" : "0001";
	if (!pattern[0].substr(0, 4).compare(code)) {
		return true;
	}
	return false;
}
