#include "StdAfx.h"
#include "AdbMonitorRequest.h"
#include "AdbDevice.h"
#include "DeviceList.h"
#include "strutil.h"
#include "Utils.h"
#include <StrSafe.h>
#include <string>
#include <vector>

using namespace std;

AdbMonitorRequest::AdbMonitorRequest(void) : AdbRequest(new AdbDevice("", ""))
{
	StringCchPrintf(TAG, sizeof(TAG), _T("AdbMonitorRequest"));
	_readTogether = false;
	ResumeThread(_listener);
}

AdbMonitorRequest::~AdbMonitorRequest(void)
{
}

bool AdbMonitorRequest::post(void)
{
	return postRequest("host:track-devices");
}

DWORD AdbMonitorRequest::maxQueueSize(void)
{
	return 1;
}

int AdbMonitorRequest::priority(void)
{
	return THREAD_PRIORITY_NORMAL;
}

void AdbMonitorRequest::readResponse(ByteBuffer* data)
{
	bool isOk = Utils::isOkay(data->buf());
	bool isFl = Utils::isFail(data->buf());

	// The response is just "OKAY" or "FAIL", return
	if (isFl || (isOk && data->size() == 4)) {
		return;
	}

	if (isOk) {
		// "OKAY" is followed by devices information.
		data->seek(4);
	}

	string resp = readResponseStr(data);

	if (resp.length() > 0 && !Utils::isOkay(resp.c_str()) && !Utils::isFail(resp.c_str())) {
		vector<Device*> list;
		vector<string> info;
		vector<string> lines = strutil::split(resp, "\n");
		for (int i = 0; i < lines.size(); i++) {
			info = strutil::split(lines[i], "\t");
			if (info.size() == 2) {
				// new adb uses only serial numbers to identify devices
				AdbDevice* device = new AdbDevice(info[0], info[1]);

				//add the device to the list
				list.push_back(device);

				info.clear();
			}
		}
		DeviceList::updateDevices(list);
		list.clear();
	}
}
