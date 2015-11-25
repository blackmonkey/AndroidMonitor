// AndroidMonitor.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "StrSafe.h"
#include "AndroidMonitor.h"
#include "Device.h"
#include "DeviceList.h"
#include "AdbMonitorRequest.h"
#include "Key.h"
#include "NetDevice.h"
#include "Utils.h"
#include "FreeImage.h"
#include <WindowsX.h>
#include <math.h>
#include <CommCtrl.h>

BOOL CALLBACK ScreenshotDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#define MAX_LOADSTRING 100
#define SWAP(a, b) (a) = (a) ^ (b); (b) = (a) ^ (b); (a) = (a) ^ (b)

// Global Variables:
HINSTANCE          hInst = NULL;                       // current instance
HWND               screenWnd = NULL;
HWND               mainDlg = NULL;
AdbMonitorRequest* deviceMonitor = NULL;
Device*            currentDevice = NULL;
DeviceList*        deviceList = NULL;
Utils*             utils = NULL;
TCHAR              szTitle[MAX_LOADSTRING] = {0};          // the title bar text
TCHAR              szWindowClass[MAX_LOADSTRING] = {0};    // the main window class name
TCHAR              TAG[16] = "AndroidMonitor";

void AlterButtonsText(HWND hDlg, BOOL toAlter)
{
	static LPTSTR btnText[30][2] = {
		{_T("1"), _T("!")},  {_T("2"), _T("@")},       {_T("3"), _T("#")},
		{_T("4"), _T("$")},  {_T("5"), _T("%")},       {_T("6"), _T("^")},
		{_T("7"), _T("&&")}, {_T("8"), _T("*")},       {_T("9"), _T("(")},
		{_T("0"), _T(")")},  {_T("Q"), _T("|")},       {_T("W"), _T("~")},
		{_T("E"), _T("\"")}, {_T("R"), _T("`")},       {_T("T"), _T("{")},
		{_T("Y"), _T("}")},  {_T("U"), _T("_")},       {_T("I"), _T("-")},
		{_T("O"), _T("+")},  {_T("P"), _T("=")},       {_T("S"), _T("\\")},
		{_T("D"), _T("'")},  {_T("F"), _T("[")},       {_T("G"), _T("]")},
		{_T("H"), _T("<")},  {_T("J"), _T(">")},       {_T("K"), _T(";")},
		{_T("L"), _T(":")},  {_T("SPACE"), _T("->|")}, {_T("/"), _T("?")}
	};
	for (int i = IDC_BTN_1; i < IDC_BTN_A; i++) {
		SetDlgItemText(hDlg, i, btnText[i - IDC_BTN_1][toAlter]);
	}
}

BOOL launchAdbServer(HWND hDlg)
{
	BOOL res = FALSE;
	TCHAR adbPath[MAX_PATH]= {0};
	TCHAR buf[BUF_LEN] = {0}; // BUF_LEN defined in Utils.h

	int len = GetModuleFileName(NULL, adbPath, MAX_PATH);
	while (adbPath[--len] != '\\') {}
	adbPath[len] = 0;
	StringCchPrintf(buf, BUF_LEN, _T("%s\\adb.exe start-server"), adbPath);

	STARTUPINFO siStartupInfo = {0};
	PROCESS_INFORMATION piProcessInfo = {0};
	siStartupInfo.cb = sizeof(siStartupInfo);

	if (CreateProcess(NULL, buf,
					  NULL, NULL, FALSE,
					  CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE,
					  0, 0, &siStartupInfo, &piProcessInfo)) {
		/* Watch the process. */
		WaitForSingleObject(piProcessInfo.hProcess, INFINITE);
		res = TRUE;
	} else {
		/* CreateProcess failed */
		int err = GetLastError();
		if (ERROR_FILE_NOT_FOUND == err || ERROR_PATH_NOT_FOUND == err) {
			StringCchPrintf(buf, BUF_LEN, _T("Cannot find %s\\adb.exe, AdbWinApi.dll, AdbWinUsbApi.dll"), adbPath);
			Utils::alertInfo(buf);
		}
		res = FALSE;
		Utils::err(TAG, "Failed to start adb server", GetLastError());
	}
	CloseHandle(piProcessInfo.hProcess);
	CloseHandle(piProcessInfo.hThread);

	return res;
}

void InitializeApp(HWND hDlg)
{
	mainDlg = hDlg;

	deviceList = new DeviceList(GetDlgItem(hDlg, IDC_DEVICE_LIST));

	utils = new Utils();
	Utils::init(GetDlgItem(hDlg, IDC_LOG), hDlg, szTitle);
	Utils::log(TAG, "Initializing WSA 2.2 ...");

	WSADATA info;
	int res = WSAStartup(MAKEWORD(2,2), &info);
	if (res != S_OK) {
		Utils::err(TAG, "WSAStartup() failed", res);
		return;
	}
	Utils::log(TAG, "WSA Done!");
	if (LOBYTE(info.wVersion) != 2 || HIBYTE(info.wVersion) != 2) {
		Utils::log(TAG, "But find WSA %d.%d, not 2.2!", LOBYTE(info.wVersion), HIBYTE(info.wVersion));
	}

	if (launchAdbServer(hDlg)) {
		deviceMonitor = new AdbMonitorRequest();
		deviceMonitor->post();
	}

	RECT rcDlg, rcScreen;
	int xOffset;

	screenWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SCREENSHOT), hDlg, (DLGPROC)ScreenshotDlgProc);

	GetWindowRect(hDlg, &rcDlg);
	GetWindowRect(screenWnd, &rcScreen);

	xOffset = (rcScreen.right - rcScreen.left) / 2;
	rcDlg.left += xOffset;
	rcDlg.right += xOffset;
	MoveWindow(hDlg,
			   rcDlg.left,
			   rcDlg.top,
			   rcDlg.right - rcDlg.left,
			   rcDlg.bottom - rcDlg.top,
			   TRUE);

	xOffset = rcDlg.left - rcScreen.right;
	rcScreen.left += xOffset;
	rcScreen.right += xOffset;
	MoveWindow(screenWnd,
			   rcScreen.left,
			   rcDlg.top,
			   rcScreen.right - rcScreen.left,
			   rcScreen.bottom - rcScreen.top,
			   TRUE);

	Button_SetCheck(GetDlgItem(screenWnd, IDC_ROTATE_0), TRUE);
	ShowWindow(screenWnd, SW_SHOW);
	FreeImage_Initialise();
}

void FinalizeApp(HWND hDlg)
{
	Utils::log(TAG, "WSACleanup()");
	FreeImage_DeInitialise();
	WSACleanup();
}

void ShowScreenshot(HWND hDlg, SIZE orgImgSize, LPARAM lParam)
{
	// no screenshot yet
	if (!lParam) {
		return;
	}

	SIZE dstImgSize = orgImgSize;
	POINT dstPos = {0};

	int rotateDegree = 0;
	XFORM xform = {0};
	if (Button_GetCheck(GetDlgItem(hDlg, IDC_ROTATE_90))) {
		xform.eM11 = 0;
		xform.eM12 = 1.0;
		xform.eM21 = -1.0;
		xform.eM22 = 0;
		xform.eDx  = (FLOAT)orgImgSize.cy;
		xform.eDy  = 0;
		rotateDegree = 90;
		dstImgSize.cx = orgImgSize.cy;
		dstImgSize.cy = orgImgSize.cx;
	} else if (Button_GetCheck(GetDlgItem(hDlg, IDC_ROTATE_180))) {
		xform.eM11 = -1.0;
		xform.eM12 = 0;
		xform.eM21 = 0;
		xform.eM22 = -1.0;
		xform.eDx  = (FLOAT)orgImgSize.cx;
		xform.eDy  = (FLOAT)orgImgSize.cy;
		rotateDegree = 180;
		dstPos.x = dstPos.y = 1;
	} else if (Button_GetCheck(GetDlgItem(hDlg, IDC_ROTATE_270))) {
		xform.eM11 = 0;
		xform.eM12 = -1.0;
		xform.eM21 = 1.0;
		xform.eM22 = 0;
		xform.eDx  = 0;
		xform.eDy  = (FLOAT)orgImgSize.cx;
		rotateDegree = 270;
		dstImgSize.cx = orgImgSize.cy;
		dstImgSize.cy = orgImgSize.cx;
	}

	HWND hCtrl = GetDlgItem(hDlg, IDC_STATIC_SCREEN);

	// Get screenshot control's information: border width, position
	WINDOWINFO wiCtrl = {0};
	wiCtrl.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(hCtrl, &wiCtrl);

	// calculate size changing of screenshot control/dialog
	SIZE change = {0};
	change.cx = dstImgSize.cx - (wiCtrl.rcClient.right - wiCtrl.rcClient.left);
	change.cy = dstImgSize.cy - (wiCtrl.rcClient.bottom - wiCtrl.rcClient.top);

	if (change.cx != 0 || change.cy != 0) {
		// Resize screenshot control
		ScreenToClient(hDlg, (LPPOINT)&wiCtrl.rcWindow);
		MoveWindow(hCtrl,
			wiCtrl.rcWindow.left,
			wiCtrl.rcWindow.top,
			dstImgSize.cx + (wiCtrl.cxWindowBorders << 1),
			dstImgSize.cy + (wiCtrl.cyWindowBorders << 1),
			TRUE);

		// Get screenshot dialog's rectangle.
		RECT rc = {0};
		GetWindowRect(hDlg, &rc);

		// Resize screenshot dialog
		rc.right += change.cx;
		rc.bottom += change.cy;
		SetWindowPos(hDlg, NULL,
			rc.left, rc.top,
			rc.right - rc.left,
			rc.bottom - rc.top,
			SWP_NOZORDER);
	}

	HDC hdc = GetDC(hCtrl);
	int oldGraphicsMode = 0;

	if (rotateDegree > 0) {
		oldGraphicsMode = SetGraphicsMode(hdc, GM_ADVANCED);
		// rotateDegree the DC
		if (!SetWorldTransform(hdc, &xform))
			return;
	}

	// show screenshot bitmap
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP oldBmp = SelectBitmap(hdcMem, (HBITMAP)lParam);
	BitBlt(hdc, dstPos.x, dstPos.y, orgImgSize.cx, orgImgSize.cy, hdcMem, 0, 0, SRCCOPY);
	SelectBitmap(hdcMem, oldBmp);

	// release screenshot bitmap
	DeleteObject((HBITMAP)lParam);
	DeleteDC(hdcMem);

	if (rotateDegree > 0) {
		ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
		SetGraphicsMode(hdc, oldGraphicsMode);
	}
	ReleaseDC(hCtrl, hdc);
}

void MapMousePoint(HWND hDlg, LPARAM lParam, LPPOINT dstPt)
{
	POINT pt = {0};
	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);

	dstPt->x = dstPt->y = 0;

	if (!ClientToScreen(hDlg, &pt)) return;

	HWND hCtrl = GetDlgItem(hDlg, IDC_STATIC_SCREEN);
	if (!ScreenToClient(hCtrl, &pt)) return;

	if (Button_GetCheck(GetDlgItem(hDlg, IDC_ROTATE_0))) {
		dstPt->x = pt.x;
		dstPt->y = pt.y;
		return;
	}

	RECT rc = {0};
	GetClientRect(hCtrl, &rc);

	if (Button_GetCheck(GetDlgItem(hDlg, IDC_ROTATE_90))) {
		dstPt->x = pt.y;
		dstPt->y = rc.right - pt.x;
		return;
	}

	if (Button_GetCheck(GetDlgItem(hDlg, IDC_ROTATE_180))) {
		dstPt->x = rc.right - pt.x;
		dstPt->y = rc.bottom - pt.y;
		return;
	}

	if (Button_GetCheck(GetDlgItem(hDlg, IDC_ROTATE_270))) {
		dstPt->x = rc.bottom - pt.y;
		dstPt->y = pt.x;
		return;
	}
}

BOOL CALLBACK AddNetDeviceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int port = 0;
	DWORD ip = 0;

	switch(message) {
	case WM_INITDIALOG:
		Edit_LimitText(GetDlgItem(hDlg, IDC_PORT), 5);
		return TRUE;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			port = GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);
			if (port < 1024 || port > 65535) {
				Utils::alertErr(_T("Port should range from 1024 to 65535!"));
			} else {
				SendDlgItemMessage(hDlg, IDC_IP, IPM_GETADDRESS, 0, (LPARAM)&ip);
				DeviceList::add(new NetDevice(ip, port));
				EndDialog(hDlg, 1);
			}
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK AddBtDeviceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case WM_INITDIALOG:
		// TODO: List connected bluetooth devices
		return TRUE;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hDlg, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK ScreenshotDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HBITMAP screenshot = NULL;
	static SIZE imgSize = {0};
	static POINT pt = {0};

	switch (message) {
	case WM_SHOW_SCREENSHOT:
		if (screenshot) {
			DeleteObject(screenshot);
		}
		screenshot = (HBITMAP)lParam;
		imgSize.cx = LOWORD(wParam);
		imgSize.cy = HIWORD(wParam);
		InvalidateRect(hDlg, NULL, FALSE);
		return TRUE;

	case WM_LBUTTONDOWN:
		MapMousePoint(hDlg, lParam, &pt);
		if (currentDevice) {
			currentDevice->pointerDown(pt.x, pt.y);
		}
		break;

	case WM_MOUSEMOVE:
		if (currentDevice && (wParam & MK_LBUTTON)) {
			MapMousePoint(hDlg, lParam, &pt);
			currentDevice->pointerMove(pt.x, pt.y);
		}
		break;

	case WM_LBUTTONUP:
		if (currentDevice) {
			currentDevice->pointerUp();
		}
		break;

	case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);
			ShowScreenshot(hDlg, imgSize, (LPARAM)screenshot);
			EndPaint(hDlg, &ps);
		}
		break;

	case WM_DESTROY:
		if (screenshot) {
			DeleteObject(screenshot);
		}
		break;
	}
	return FALSE;
}

//
//  FUNCTION: DlgProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
BOOL CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) {
	case WM_INITDIALOG:
		InitializeApp(hDlg);
		return TRUE;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId) {
		case IDC_DEVICE_LIST:
			switch (wmEvent) {
			case LBN_SELCHANGE:
				HWND hList = (HWND)lParam;
				int i = ListBox_GetCurSel(hList);
				Device* device = (Device*)ListBox_GetItemData(hList, i);
				if (device && device != currentDevice) {
					// switch device
					if (currentDevice) {
						currentDevice->stopScreenshot();
					}
					DeviceList::monitorDevice(i);
					currentDevice = device;

					device->setOutputWnd(screenWnd, GetDlgItem(screenWnd, IDC_STATIC_SCREEN));
					if (device->getState() == Device::ONLINE || device->getState() == Device::READY) {
						device->startScreenshot();
					}
				}
				return TRUE;
			}
			break;

		case IDC_BTN_ADD_NET_DEVICE:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ADD_NET_DEVICE), hDlg, AddNetDeviceDlgProc);
			break;

		case IDC_BTN_ADD_BT_DEVICE:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ADD_BT_DEVICE), hDlg, AddBtDeviceDlgProc);
			break;

		case IDC_BTN_CAMERA:      if (currentDevice) {currentDevice->keyStrike(KEYCODE_CAMERA);} return TRUE;
		case IDC_BTN_VOLUME_DOWN: if (currentDevice) {currentDevice->keyStrike(KEYCODE_VOLUME_DOWN);} return TRUE;
		case IDC_BTN_VOLUME_UP:   if (currentDevice) {currentDevice->keyStrike(KEYCODE_VOLUME_UP);} return TRUE;
		case IDC_BTN_POWER:       if (currentDevice) {currentDevice->keyStrike(KEYCODE_POWER);} return TRUE;
		case IDC_BTN_UP:          if (currentDevice) {currentDevice->keyStrike(KEYCODE_DPAD_UP);} return TRUE;
		case IDC_BTN_CALL:        if (currentDevice) {currentDevice->keyStrike(KEYCODE_CALL);} return TRUE;
		case IDC_BTN_LEFT:        if (currentDevice) {currentDevice->keyStrike(KEYCODE_DPAD_LEFT);} return TRUE;
		case IDC_BTN_SELECT:      if (currentDevice) {currentDevice->keyStrike(KEYCODE_DPAD_CENTER);} return TRUE;
		case IDC_BTN_RIGHT:       if (currentDevice) {currentDevice->keyStrike(KEYCODE_DPAD_RIGHT);} return TRUE;
		case IDC_BTN_END:         if (currentDevice) {currentDevice->keyStrike(KEYCODE_ENDCALL);} return TRUE;
		case IDC_BTN_DOWN:        if (currentDevice) {currentDevice->keyStrike(KEYCODE_DPAD_DOWN);} return TRUE;
		case IDC_BTN_HOME:        if (currentDevice) {currentDevice->keyStrike(KEYCODE_HOME);} return TRUE;
		case IDC_BTN_MENU:        if (currentDevice) {currentDevice->keyStrike(KEYCODE_MENU);} return TRUE;
		case IDC_BTN_BACK:        if (currentDevice) {currentDevice->keyStrike(KEYCODE_BACK);} return TRUE;
		case IDC_BTN_SEARCH:      if (currentDevice) {currentDevice->keyStrike(KEYCODE_SEARCH);} return TRUE;
		case IDC_BTN_1:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_1);} return TRUE;
		case IDC_BTN_2:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_2);} return TRUE;
		case IDC_BTN_3:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_3);} return TRUE;
		case IDC_BTN_4:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_4);} return TRUE;
		case IDC_BTN_5:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_5);} return TRUE;
		case IDC_BTN_6:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_6);} return TRUE;
		case IDC_BTN_7:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_7);} return TRUE;
		case IDC_BTN_8:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_8);} return TRUE;
		case IDC_BTN_9:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_9);} return TRUE;
		case IDC_BTN_0:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_0);} return TRUE;
		case IDC_BTN_Q:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_Q);} return TRUE;
		case IDC_BTN_W:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_W);} return TRUE;
		case IDC_BTN_E:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_E);} return TRUE;
		case IDC_BTN_R:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_R);} return TRUE;
		case IDC_BTN_T:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_T);} return TRUE;
		case IDC_BTN_Y:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_Y);} return TRUE;
		case IDC_BTN_U:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_U);} return TRUE;
		case IDC_BTN_I:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_I);} return TRUE;
		case IDC_BTN_O:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_O);} return TRUE;
		case IDC_BTN_P:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_P);} return TRUE;
		case IDC_BTN_A:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_A);} return TRUE;
		case IDC_BTN_S:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_S);} return TRUE;
		case IDC_BTN_D:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_D);} return TRUE;
		case IDC_BTN_F:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_F);} return TRUE;
		case IDC_BTN_G:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_G);} return TRUE;
		case IDC_BTN_H:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_H);} return TRUE;
		case IDC_BTN_J:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_J);} return TRUE;
		case IDC_BTN_K:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_K);} return TRUE;
		case IDC_BTN_L:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_L);} return TRUE;
		case IDC_BTN_DEL:         if (currentDevice) {currentDevice->keyStrike(KEYCODE_DEL);} return TRUE;
		case IDC_BTN_SHIFT:       if (currentDevice) {currentDevice->keyStrike(KEYCODE_SHIFT_LEFT);} return TRUE;
		case IDC_BTN_Z:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_Z);} return TRUE;
		case IDC_BTN_X:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_X);} return TRUE;
		case IDC_BTN_C:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_C);} return TRUE;
		case IDC_BTN_V:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_V);} return TRUE;
		case IDC_BTN_B:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_B);} return TRUE;
		case IDC_BTN_N:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_N);} return TRUE;
		case IDC_BTN_M:           AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_M);} return TRUE;
		case IDC_BTN_DOT:         AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_PERIOD);} return TRUE;
		case IDC_BTN_ENTER:       if (currentDevice) {currentDevice->keyStrike(KEYCODE_ENTER);} return TRUE;
		case IDC_BTN_LALT:        AlterButtonsText(hDlg, TRUE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_ALT_LEFT);} return TRUE;
		case IDC_BTN_SYMBOL:      if (currentDevice) {currentDevice->keyStrike(KEYCODE_SYM);} return TRUE;
		case IDC_BTN_AT:          AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_AT);} return TRUE;
		case IDC_BTN_SPACE:       AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_SPACE);} return TRUE;
		case IDC_BTN_SLASH:       AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_SLASH);} return TRUE;
		case IDC_BTN_COMMA:       AlterButtonsText(hDlg, FALSE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_COMMA);} return TRUE;
		case IDC_BTN_RALT:        AlterButtonsText(hDlg, TRUE); if (currentDevice) {currentDevice->keyStrike(KEYCODE_ALT_RIGHT);} return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	case WM_DESTROY:
		if (currentDevice) {
			currentDevice->stopScreenshot();
		}
		if (deviceMonitor) {
			delete deviceMonitor;
		}
		if (utils) {
			delete utils;
		}
		if (deviceList) {
			delete deviceList;
		}

		DestroyWindow(screenWnd);
		PostQuitMessage(0);
		break;
	}

	return FALSE;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= DefDlgProc;
	wcex.cbWndExtra		= DLGWINDOWEXTRA;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ANDROIDMONITOR));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (hPrevInstance) {
		// TODO: Instead of popping warning message, bring the window
		// of hPrevInstance to foreground.
		Utils::alertInfo(_T("Only one instance is allowed to be launched at same runtime!"));
		return FALSE;
	}

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_ANDROIDMONITOR, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	hInst = hInstance; // Store instance handle in our global variable

	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)DlgProc);
}
