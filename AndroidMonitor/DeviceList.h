#pragma once

#include "Windows.h"
#include "Device.h"
#include <vector>

using namespace std;

class DeviceList
{
public:
	DeviceList(HWND hCtrl);
	~DeviceList(void);
	static void monitorDevice(int index);
	static void updateDevices(vector<Device*> list);
	static void add(Device* dev);
	static void remove(Device* dev);

private:
	static HWND outCtrl;   // The control to output device list
	static vector<Device*> deviceList;

	static void updateList(void);
};
