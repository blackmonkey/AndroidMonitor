#include "StdAfx.h"
#include "DeviceList.h"
#include "Utils.h"
#include <WindowsX.h>

HWND DeviceList::outCtrl;   // The control to output device list
vector<Device*> DeviceList::deviceList;

DeviceList::DeviceList(HWND hCtrl)
{
	outCtrl = hCtrl;
}

DeviceList::~DeviceList(void)
{
	while (deviceList.size() > 0) {
		deviceList.erase(deviceList.begin());
	}
}

/**
 * Update UI List
 */
void DeviceList::updateList(void)
{
	while (ListBox_GetCount(outCtrl) > 0) {
		ListBox_DeleteString(outCtrl, 0);
	}
	for (int i = 0; i < deviceList.size(); i++) {
		ListBox_AddString(outCtrl, deviceList[i]->getDescription().c_str());
		ListBox_SetItemData(outCtrl, i, deviceList[i]);
		if (deviceList[i]->isMonitoring()) {
			ListBox_SetCurSel(outCtrl, i);
		}
	}
}

/**
 * Updates the device list with the new items received from the monitoring service.
 */
void DeviceList::updateDevices(std::vector<Device*> list)
{
	int i, j;
	BOOL foundMatch;

	// Update current recorded devices
	for (i = 0; i < deviceList.size();) {
		foundMatch = FALSE;
		for (j = 0; j < list.size(); j++) {
			if (!deviceList[i]->getName().compare(list[j]->getName())) {
				foundMatch = TRUE;
				if (deviceList[i]->getState() != list[j]->getState()) {
					deviceList[i]->setState(list[j]->getState());
				}

				// remove the new device from the list since it's been used
				list.erase(list.begin() + j);
			}
		}

		if (!foundMatch && deviceList[i]->getType() == Device::ADB_DEVICE) {
			// the device is gone, we need to remove it, and keep current index
			// to process the next one.
			deviceList.erase(deviceList.begin() + i);
		} else {
			i++;
		}
	}

	// Add new devices
	for (i = 0; i < list.size(); i++) {
		deviceList.push_back(list[i]);
	}

	updateList();
}

void DeviceList::monitorDevice(int index)
{
	for (int i = 0; i < deviceList.size(); i++) {
		deviceList[i]->setMonitoring(i == index);
	}
}

void DeviceList::add(Device* dev)
{
	for (int i = 0; i < deviceList.size(); i++) {
		if (deviceList[i] == dev || !deviceList[i]->getName().compare(dev->getName())) {
			Utils::alertInfo(_T("The device is already connected!"));
			return;
		}
	}
	deviceList.push_back(dev);
	updateList();
}

void DeviceList::remove(Device* dev)
{
	for (int i = 0; i < deviceList.size();) {
		if (deviceList[i] == dev || !deviceList[i]->getName().compare(dev->getName())) {
			deviceList.erase(deviceList.begin() + i);
		} else {
			i++;
		}
	}
	updateList();
}
