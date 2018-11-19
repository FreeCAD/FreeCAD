/***************************************************************************
 *   Copyright (c) 2018 Torsten Sadowski <tsadowski[at]gmx.net>            *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

/*
Development tools and related technology provided under license from 3Dconnexion.
(c) 1992 - 2012 3Dconnexion. All rights reserved
*/

/*
Support for Qt added by David Dibben at
http://www.codegardening.com/2011/02/using-3dconnexion-mouse-with-qt.html
*/

/*
See also:
http://www.3dconnexion.com/forum/viewtopic.php?f=19&t=4968&sid=72c018bdcf0e6edc99a6effb5c0c48d9
*/

#include "PreCompiled.h"

#include "GuiNativeEventWin32.h"

#include <QGlobalStatic>
#include <QMainWindow>
#include <QWidget>
#include <FCConfig.h>
#include <Base/Console.h>
#include "GuiApplicationNativeEventAware.h"
#if QT_VERSION >= 0x050000
  #include "GuiRawInputEventFilter.h"
#endif // #if QT_VERSION >= 0x050000

std::vector<int> Gui::GuiNativeEvent::motionDataArray(6,0);
Gui::GuiNativeEvent* Gui::GuiNativeEvent::gMouseInput = 0;


// Windows dependencies, enumerators and global variables

#include <QApplication>
#include <windows.h>
#include <cmath>

#define LOGITECH_VENDOR_ID 0x46d
#define CONNEXION_VENDOR_ID  0x256f
#define _CONSTANT_INPUT_PERIOD 0

#ifndef RIDEV_DEVNOTIFY
#define RIDEV_DEVNOTIFY 0x00002000
#endif

#define _TRACE_WM_INPUT_PERIOD 0
#define _TRACE_RI_TYPE 0
#define _TRACE_RIDI_DEVICENAME 0
#define _TRACE_RIDI_DEVICEINFO 0
#define _TRACE_RI_RAWDATA 0
#define _TRACE_3DINPUT_PERIOD 0

#ifdef _WIN64
typedef unsigned __int64 QWORD;
#endif // _WIN64

static const int kTimeToLive = 5;

enum e3dconnexion_pid {
   eSpacePilot = 0xc625,
   eSpaceNavigator = 0xc626,
   eSpaceExplorer = 0xc627,
   eSpaceNavigatorForNotebooks = 0xc628,
   eSpacePilotPRO = 0xc629,
   eSpaceMouseWireless = 0xc62e,
   eSpaceMouseWirelessReceiver = 0xc62f,
   eSpaceMousePROWireless = 0xC631,
   eSpaceMousePROWirelessReceiver = 0xC632
};

enum e3dmouse_virtual_key
{
   V3DK_INVALID=0
   , V3DK_MENU=1, V3DK_FIT
   , V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT, V3DK_BOTTOM, V3DK_BACK
   , V3DK_CW, V3DK_CCW
   , V3DK_ISO1, V3DK_ISO2
   , V3DK_1, V3DK_2, V3DK_3, V3DK_4, V3DK_5, V3DK_6, V3DK_7, V3DK_8, V3DK_9, V3DK_10
   , V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL
   , V3DK_ROTATE, V3DK_PANZOOM, V3DK_DOMINANT
   , V3DK_PLUS, V3DK_MINUS
};

struct tag_VirtualKeys
{
   e3dconnexion_pid pid;
   size_t nKeys;
   e3dmouse_virtual_key *vkeys;
};

static const e3dmouse_virtual_key SpaceMouseWirelessKeys [] =
{
   V3DK_INVALID     // there is no button 0
   , V3DK_1, V3DK_2
   , V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT
   , V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL
   , V3DK_FIT, V3DK_MENU
   , V3DK_PLUS, V3DK_MINUS
   , V3DK_ROTATE
};

static const e3dmouse_virtual_key SpaceMouseWirelessReceiverKeys [] =
{
   V3DK_INVALID     // there is no button 0
   , V3DK_1, V3DK_2
   , V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT
   , V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL
   , V3DK_FIT, V3DK_MENU
   , V3DK_PLUS, V3DK_MINUS
   , V3DK_ROTATE
};

static const e3dmouse_virtual_key SpaceExplorerKeys [] =
{
   V3DK_INVALID     // there is no button 0
   , V3DK_1, V3DK_2
   , V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT
   , V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL
   , V3DK_FIT, V3DK_MENU
   , V3DK_PLUS, V3DK_MINUS
   , V3DK_ROTATE
};

static const e3dmouse_virtual_key SpacePilotKeys [] =
{
   V3DK_INVALID
   , V3DK_1, V3DK_2, V3DK_3, V3DK_4, V3DK_5, V3DK_6
   , V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT
   , V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL
   , V3DK_FIT, V3DK_MENU
   , V3DK_PLUS, V3DK_MINUS
   , V3DK_DOMINANT, V3DK_ROTATE
};


static const struct tag_VirtualKeys _3dmouseVirtualKeys[]=
{
   eSpacePilot
   , sizeof(SpacePilotKeys)/sizeof(SpacePilotKeys[0])
   , const_cast<e3dmouse_virtual_key *>(SpacePilotKeys),
   eSpaceExplorer
   , sizeof(SpaceExplorerKeys)/sizeof(SpaceExplorerKeys[0])
   , const_cast<e3dmouse_virtual_key *>(SpaceExplorerKeys),
   eSpaceMouseWireless
   , sizeof(SpaceMouseWirelessKeys)/sizeof(SpaceMouseWirelessKeys[0])
   , const_cast<e3dmouse_virtual_key *>(SpaceMouseWirelessKeys),
   eSpaceMouseWirelessReceiver
   , sizeof(SpaceMouseWirelessReceiverKeys)/sizeof(SpaceMouseWirelessReceiverKeys[0])
   , const_cast<e3dmouse_virtual_key *>(SpaceMouseWirelessReceiverKeys),
   eSpaceMousePROWireless
   , sizeof(SpaceMouseWirelessKeys)/sizeof(SpaceMouseWirelessKeys[0])
   , const_cast<e3dmouse_virtual_key *>(SpaceMouseWirelessKeys),
   eSpaceMousePROWirelessReceiver
   , sizeof(SpaceMouseWirelessReceiverKeys)/sizeof(SpaceMouseWirelessReceiverKeys[0])
   , const_cast<e3dmouse_virtual_key *>(SpaceMouseWirelessReceiverKeys)
};

Gui::GuiNativeEvent::GuiNativeEvent(Gui::GUIApplicationNativeEventAware *app)
{
	mainApp = app;
}

Gui::GuiNativeEvent::~GuiNativeEvent()
{
    if (gMouseInput == this) {
        gMouseInput = 0;
        Base::Console().Log("3Dconnexion device detached.\n");
    }
}

void Gui::GuiNativeEvent::initSpaceball(QMainWindow *mainWindow)
{
    mainApp->setSpaceballPresent(Is3dmouseAttached());

    if (mainApp->isSpaceballPresent()) {
        fLast3dmouseInputTime = 0;

        if (InitializeRawInput((HWND)mainWindow->winId())){
            gMouseInput = this;
#if QT_VERSION >= 0x050000
            qApp->installNativeEventFilter(new Gui::RawInputEventFilter(Gui::GuiNativeEvent::RawInputEventFilter));
#else
            qApp->setEventFilter(Gui::GuiNativeEvent::RawInputEventFilter);
#endif
            Base::Console().Log("3Dconnexion device initialized.\n");
        } else {
            Base::Console().Log("3Dconnexion device is attached, but not initialized.\n");
        }
    } else {
        Base::Console().Log("3Dconnexion device not attached.\n");
    }
}

// Methods for windows events

/*!
	Converts a hid device keycode (button identifier) of a pre-2009 3Dconnexion USB device to the standard 3d mouse virtual key definition.

	\a pid USB Product ID (PID) of 3D mouse device
	\a hidKeyCode  Hid keycode as retrieved from a Raw Input packet

	\return The standard 3d mouse virtual key (button identifier) or zero if an error occurs.

	Converts a hid device keycode (button identifier) of a pre-2009 3Dconnexion USB device
	to the standard 3d mouse virtual key definition.
*/

unsigned short HidToVirtualKey(unsigned long pid, unsigned short hidKeyCode)
{
   unsigned short virtualkey=hidKeyCode;
   for (size_t i=0; i<sizeof(_3dmouseVirtualKeys)/sizeof(_3dmouseVirtualKeys[0]); ++i)
   {
	  if (pid == _3dmouseVirtualKeys[i].pid)
	  {
		 if (hidKeyCode < _3dmouseVirtualKeys[i].nKeys)
			virtualkey = _3dmouseVirtualKeys[i].vkeys[hidKeyCode];
		 else
			virtualkey = V3DK_INVALID;
		 break;
	  }
   }
   // Remaining devices are unchanged
   return virtualkey;
}


bool Gui::GuiNativeEvent::RawInputEventFilter(void* msg, long* result)
{
	if (gMouseInput == 0) return false;

	MSG* message = (MSG*)(msg);

	if (message->message == WM_INPUT) {
		HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>(message->lParam);
		gMouseInput->OnRawInput(RIM_INPUT,hRawInput);
		if (result != 0)  {
			result = 0;
		}
		return true;
	}

	return false;
}


/*!
	Access the mouse parameters structure
*/
I3dMouseParam& Gui::GuiNativeEvent::MouseParams()
{
	return f3dMouseParams;
}

/*!
	Access the mouse parameters structure
*/
const I3dMouseParam& Gui::GuiNativeEvent::MouseParams() const
{
	return f3dMouseParams;
}

/*!
	Called with the processed motion data when a 3D mouse event is received

	The default implementation emits a Move3d signal with the motion data
*/
void Gui::GuiNativeEvent::Move3d(HANDLE device, std::vector<float>& motionData)
{
	Q_UNUSED(device);
	
    motionDataArray[0] = -ceil(motionData[0]);
    motionDataArray[1] = ceil(motionData[1]);
    motionDataArray[2] = ceil(motionData[2]);
    motionDataArray[3] = -ceil(motionData[3]);
    motionDataArray[4] = ceil(motionData[4]);
    motionDataArray[5] = ceil(motionData[5]);

    mainApp->postMotionEvent(motionDataArray);
}

/*!
	Called when a 3D mouse key is pressed

	The default implementation emits a On3dmouseKeyDown signal with the key code.
*/
void Gui::GuiNativeEvent::On3dmouseKeyDown(HANDLE device, int virtualKeyCode)
{
	Q_UNUSED(device);
    
    mainApp->postButtonEvent(virtualKeyCode - 1, 1);
}

/*!
	Called when a 3D mouse key is released

	The default implementation emits a On3dmouseKeyUp signal with the key code.
*/
void Gui::GuiNativeEvent::On3dmouseKeyUp(HANDLE device, int virtualKeyCode)
{
	Q_UNUSED(device);
    
    mainApp->postButtonEvent(virtualKeyCode - 1, 0);
}

/*!
	Get an initialized array of PRAWINPUTDEVICE for the 3D devices

	pNumDevices returns the number of devices to register. Currently this is always 1.
 */
static PRAWINPUTDEVICE GetDevicesToRegister(unsigned int* pNumDevices)
{
    // Array of raw input devices to register
    static RAWINPUTDEVICE sRawInputDevices[] = {
        {0x01, 0x08, 0x00, 0x00} // Usage Page = 0x01 Generic Desktop Page, Usage Id= 0x08 Multi-axis Controller
       ,{0x01, 0x05, 0x00, 0x00} // game pad
       ,{0x01, 0x04, 0x00, 0x00} // joystick
    };

    if (pNumDevices) {
        *pNumDevices = sizeof(sRawInputDevices) / sizeof(sRawInputDevices[0]);
    }

    return sRawInputDevices;
}

/*!
	Detect the 3D mouse
*/
bool Gui::GuiNativeEvent::Is3dmouseAttached()
{
	unsigned int numDevicesOfInterest = 0;
	PRAWINPUTDEVICE devicesToRegister = GetDevicesToRegister(&numDevicesOfInterest);

	unsigned int nDevices = 0;

	if (::GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0) {
		return false;
	}

	if (nDevices == 0) return false;

	std::vector<RAWINPUTDEVICELIST> rawInputDeviceList(nDevices);
	if (::GetRawInputDeviceList(&rawInputDeviceList[0], &nDevices, sizeof(RAWINPUTDEVICELIST)) == static_cast<unsigned int>(-1)) {
		return false;
	}

	for (unsigned int i = 0; i < nDevices; ++i) {
		RID_DEVICE_INFO rdi = {sizeof(rdi)};
		unsigned int cbSize = sizeof(rdi);

		if (GetRawInputDeviceInfo(rawInputDeviceList[i].hDevice, RIDI_DEVICEINFO, &rdi, &cbSize) > 0) {
			//skip non HID and non logitec (3DConnexion) devices
			if (!(rdi.dwType == RIM_TYPEHID && (rdi.hid.dwVendorId == LOGITECH_VENDOR_ID || rdi.hid.dwVendorId == CONNEXION_VENDOR_ID))) {
			continue;
			}

			//check if devices matches Multi-axis Controller
			for (unsigned int j = 0; j < numDevicesOfInterest; ++j) {
				if (devicesToRegister[j].usUsage == rdi.hid.usUsage
						&& devicesToRegister[j].usUsagePage == rdi.hid.usUsagePage) {
					return true;
				}
			}
		}
	}
	return false;
}



/*!
	Initialize the window to receive raw-input messages

	This needs to be called initially so that Windows will send the messages from the 3D mouse to the window.
*/
bool Gui::GuiNativeEvent::InitializeRawInput(HWND hwndTarget)
{
	fWindow = hwndTarget;

	// Simply fail if there is no window
	if (!hwndTarget)  return false;

	unsigned int numDevices = 0;
	PRAWINPUTDEVICE devicesToRegister = GetDevicesToRegister(&numDevices);

	if (numDevices == 0) return false;

	// Get OS version.
	OSVERSIONINFO osvi = {sizeof(OSVERSIONINFO),0};
	::GetVersionEx(&osvi);

	unsigned int cbSize = sizeof (devicesToRegister[0]);
	for (size_t i = 0; i < numDevices; i++) {
		// Set the target window to use
		//devicesToRegister[i].hwndTarget = hwndTarget;

		// If Vista or newer, enable receiving the WM_INPUT_DEVICE_CHANGE message.
		if (osvi.dwMajorVersion >= 6) {
			devicesToRegister[i].dwFlags |= RIDEV_DEVNOTIFY;
		}
	}
	return (::RegisterRawInputDevices(devicesToRegister, numDevices, cbSize) != FALSE);
}


/*!
	Get the raw input data from Windows

	Includes workaround for incorrect alignment of the RAWINPUT structure on x64 os
	when running as Wow64 (copied directly from 3DConnexion code)
*/

UINT Gui::GuiNativeEvent::GetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader)
{
#ifdef _WIN64
	return ::GetRawInputBuffer(pData, pcbSize, cbSizeHeader);
#else
	BOOL bIsWow64 = FALSE;
	// 0002287: Entry Point Not Found IsWow64Process for Windows 2000
	//::IsWow64Process(GetCurrentProcess(), &bIsWow64);
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
		GetModuleHandle("kernel32"), "IsWow64Process");

	if (NULL != fnIsWow64Process) {
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	}
	if (!bIsWow64 || pData==NULL) {
		return ::GetRawInputBuffer(pData, pcbSize, cbSizeHeader);
	} else {
		HWND hwndTarget = fWindow; //fParent->winId();

		size_t cbDataSize=0;
		UINT nCount=0;
		PRAWINPUT pri = pData;

		MSG msg;
		while (PeekMessage(&msg, hwndTarget, WM_INPUT, WM_INPUT, PM_NOREMOVE)) {
			HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>(msg.lParam);
			size_t cbSize = *pcbSize - cbDataSize;
			if (::GetRawInputData(hRawInput, RID_INPUT, pri, &cbSize, cbSizeHeader) == static_cast<UINT>(-1)) {
				if (nCount==0) {
					return static_cast<UINT>(-1);
				}  else {
					break;
				}
			}
			++nCount;

			// Remove the message for the data just read
			PeekMessage(&msg, hwndTarget, WM_INPUT, WM_INPUT, PM_REMOVE);

			pri = NEXTRAWINPUTBLOCK(pri);
			cbDataSize = reinterpret_cast<ULONG_PTR>(pri) - reinterpret_cast<ULONG_PTR>(pData);
			if (cbDataSize >= *pcbSize) {
				cbDataSize = *pcbSize;
				break;
			}
		}
		return nCount;
	}
#endif
}

/*!
	Process the raw input device data

	On3dmouseInput() does all the preprocessing of the rawinput device data before
	finally calling the Move3d method.
*/

void Gui::GuiNativeEvent::On3dmouseInput()
{
	// Don't do any data processing in background
	bool bIsForeground = (::GetActiveWindow() != NULL);
	if (!bIsForeground) {
		// set all cached data to zero so that a zero event is seen and the cached data deleted
		for (std::map<HANDLE, TInputData>::iterator it = fDevice2Data.begin(); it != fDevice2Data.end(); it++) {
			it->second.fAxes.assign(6, .0);
			it->second.fIsDirty = true;
		}
	}

	DWORD dwNow = ::GetTickCount();           // Current time;
	DWORD dwElapsedTime;                      // Elapsed time since we were last here

	if (0 == fLast3dmouseInputTime) {
		dwElapsedTime = 10;                    // System timer resolution
	} else {
		dwElapsedTime = dwNow - fLast3dmouseInputTime;
		if (fLast3dmouseInputTime > dwNow) {
			dwElapsedTime = ~dwElapsedTime+1;
		}
		if (dwElapsedTime<1) {
			dwElapsedTime=1;
		} else if (dwElapsedTime > 500) {
			// Check for wild numbers because the device was removed while sending data
			dwElapsedTime = 10;
		}
	}

#if _TRACE_3DINPUT_PERIOD
	qDebug("On3DmouseInput() period is %dms\n", dwElapsedTime);
#endif

	float mouseData2Rotation;
	// v = w * r,  we don't know r yet so lets assume r=1.)
	float mouseData2PanZoom;

	// Grab the I3dmouseParam interface
	 I3dMouseParam& i3dmouseParam = f3dMouseParams;
	 // Take a look at the users preferred speed setting and adjust the sensitivity accordingly
	 I3dMouseSensor::ESpeed speedSetting = i3dmouseParam.GetSpeed();
	 // See "Programming for the 3D Mouse", Section 5.1.3
	float speed = (speedSetting == I3dMouseSensor::kLowSpeed ? 0.25f : speedSetting == I3dMouseSensor::kHighSpeed ?  4.f : 1.f);

	 // Multiplying by the following will convert the 3d mouse data to real world units
	mouseData2PanZoom = speed;
	mouseData2Rotation = speed;

	std::map<HANDLE, TInputData>::iterator iterator=fDevice2Data.begin();
	while (iterator != fDevice2Data.end()) {

		// If we have not received data for a while send a zero event
		if ((--(iterator->second.fTimeToLive)) == 0) {
			iterator->second.fAxes.assign(6, .0);
		} else if (/*!t_bPoll3dmouse &&*/ !iterator->second.fIsDirty) {
		  // If we are not polling then only handle the data that was actually received
			++iterator;
			continue;
		}
		iterator->second.fIsDirty=false;

		// get a copy of the device
		HANDLE hdevice = iterator->first;

		// get a copy of the motion vectors and apply the user filters
		std::vector<float> motionData = iterator->second.fAxes;

		// apply the user filters

		// Pan Zoom filter
		// See "Programming for the 3D Mouse", Section 5.1.2
		if (!i3dmouseParam.IsPanZoom()) {
			// Pan zoom is switched off so set the translation vector values to zero
			motionData[0] =  motionData[1] =  motionData[2] = 0.;
		}

		// Rotate filter
		// See "Programming for the 3D Mouse", Section 5.1.1
		if (!i3dmouseParam.IsRotate()) {
			// Rotate is switched off so set the rotation vector values to zero
			motionData[3] =  motionData[4] =  motionData[5] = 0.;
		}

		// convert the translation vector into physical data
		for (int axis = 0; axis < 3; axis++) {
			 motionData[axis] *= mouseData2PanZoom;
		 }
		// convert the directed Rotate vector into physical data
		// See "Programming for the 3D Mouse", Section 7.2.2
		for (int axis = 3; axis < 6; axis++) {
			motionData[axis] *= mouseData2Rotation;
		}

		// Now that the data has had the filters and sensitivty settings applied
		// calculate the displacements since the last view update
		for (int axis = 0; axis < 6; axis++) {
			motionData[axis] *= dwElapsedTime;
		}


		// Now a bit of book keeping before passing on the data
		if (iterator->second.IsZero()) {
			iterator = fDevice2Data.erase(iterator);
		} else {
			++iterator;
		}

		// Work out which will be the next device
		HANDLE hNextDevice = 0;
		if (iterator != fDevice2Data.end()) {
			hNextDevice = iterator->first;
		}

		 // Pass the 3dmouse input to the view controller
		 Move3d(hdevice, motionData);

		// Because we don't know what happened in the previous call, the cache might have
		// changed so reload the iterator
		iterator = fDevice2Data.find(hNextDevice);
   }

	if (!fDevice2Data.empty()) {
		fLast3dmouseInputTime = dwNow;
	} else {
		fLast3dmouseInputTime = 0;
	}
}

/*!
	Called when new raw input data is available
*/
void Gui::GuiNativeEvent::OnRawInput(UINT nInputCode, HRAWINPUT hRawInput)
{
	const size_t cbSizeOfBuffer=1024;
	BYTE pBuffer[cbSizeOfBuffer];

	PRAWINPUT pRawInput = reinterpret_cast<PRAWINPUT>(pBuffer);
	UINT cbSize = cbSizeOfBuffer;

	if (::GetRawInputData(hRawInput, RID_INPUT, pRawInput, &cbSize, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1)) {
		return;
	}

	bool b3dmouseInput = TranslateRawInputData(nInputCode, pRawInput);
	::DefRawInputProc(&pRawInput, 1, sizeof(RAWINPUTHEADER));

	// Check for any buffered messages
	cbSize = cbSizeOfBuffer;
	UINT nCount = this->GetRawInputBuffer(pRawInput, &cbSize, sizeof(RAWINPUTHEADER));
	if (nCount == (UINT)-1) {
		 qDebug ("GetRawInputBuffer returned error %d\n", GetLastError());
	}

	while (nCount>0 && nCount !=  static_cast<UINT>(-1)) {
		PRAWINPUT pri = pRawInput;
		UINT nInput;
		for (nInput=0; nInput<nCount; ++nInput) {
			b3dmouseInput |= TranslateRawInputData(nInputCode, pri);
			// clean the buffer
			::DefRawInputProc(&pri, 1, sizeof(RAWINPUTHEADER));

			pri = NEXTRAWINPUTBLOCK(pri);
		}
		cbSize = cbSizeOfBuffer;
		nCount = this->GetRawInputBuffer(pRawInput, &cbSize, sizeof(RAWINPUTHEADER));
	}

	// If we have mouse input data for the app then tell the app about it
	if (b3dmouseInput) {
		On3dmouseInput();
	}
}



bool Gui::GuiNativeEvent::TranslateRawInputData(UINT nInputCode, PRAWINPUT pRawInput)
{
	bool bIsForeground = (nInputCode == RIM_INPUT);

#if _TRACE_RI_TYPE
	qDebug("Rawinput.header.dwType=0x%x\n", pRawInput->header.dwType);
#endif
	// We are not interested in keyboard or mouse data received via raw input
	if (pRawInput->header.dwType != RIM_TYPEHID) return false;

#if _TRACE_RIDI_DEVICENAME
	UINT dwSize=0;
	if (::GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICENAME, NULL, &dwSize) == 0)  {
		std::vector<wchar_t> szDeviceName(dwSize+1);
		if (::GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICENAME, &szDeviceName[0],	&dwSize) >0) {
			qDebug("Device Name = %s\nDevice handle = 0x%x\n", &szDeviceName[0], pRawInput->header.hDevice);
		}
   }
#endif

	RID_DEVICE_INFO sRidDeviceInfo;
	sRidDeviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
	UINT cbSize = sizeof(RID_DEVICE_INFO);

	if (::GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICEINFO, &sRidDeviceInfo, &cbSize) == cbSize) {
#if _TRACE_RIDI_DEVICEINFO
		switch (sRidDeviceInfo.dwType)  {
			case RIM_TYPEMOUSE:
				qDebug("\tsRidDeviceInfo.dwType=RIM_TYPEMOUSE\n");
				break;
			case RIM_TYPEKEYBOARD:
				qDebug("\tsRidDeviceInfo.dwType=RIM_TYPEKEYBOARD\n");
				break;
			case RIM_TYPEHID:
				qDebug("\tsRidDeviceInfo.dwType=RIM_TYPEHID\n");
				qDebug("\tVendor=0x%x\n\tProduct=0x%x\n\tUsagePage=0x%x\n\tUsage=0x%x\n",
						sRidDeviceInfo.hid.dwVendorId,
						sRidDeviceInfo.hid.dwProductId,
						sRidDeviceInfo.hid.usUsagePage,
						sRidDeviceInfo.hid.usUsage);
				break;
		}
#endif

		if (sRidDeviceInfo.hid.dwVendorId == LOGITECH_VENDOR_ID  || sRidDeviceInfo.hid.dwVendorId == CONNEXION_VENDOR_ID) {
			if (pRawInput->data.hid.bRawData[0] == 0x01) { // Translation vector

				TInputData& deviceData = fDevice2Data[pRawInput->header.hDevice];
				deviceData.fTimeToLive = kTimeToLive;
				if (bIsForeground) {
					short* pnRawData = reinterpret_cast<short*>(&pRawInput->data.hid.bRawData[1]);
					// Cache the pan zoom data
					deviceData.fAxes[0] = static_cast<float>(pnRawData[0]);
					deviceData.fAxes[1] = static_cast<float>(pnRawData[1]);
					deviceData.fAxes[2] = static_cast<float>(pnRawData[2]);

#if _TRACE_RI_RAWDATA
					qDebug("Pan/Zoom RI Data =\t0x%x,\t0x%x,\t0x%x\n",
									pnRawData[0],  pnRawData[1],  pnRawData[2]);
#endif
					if (pRawInput->data.hid.dwSizeHid >= 13) {// Highspeed package
						// Cache the rotation data
						deviceData.fAxes[3] = static_cast<float>(pnRawData[3]);
						deviceData.fAxes[4] = static_cast<float>(pnRawData[4]);
						deviceData.fAxes[5] = static_cast<float>(pnRawData[5]);
						deviceData.fIsDirty = true;
#if _TRACE_RI_RAWDATA
						qDebug("Rotation RI Data =\t0x%x,\t0x%x,\t0x%x\n",
							 pnRawData[3], pnRawData[4], pnRawData[5]);
#endif
						return true;
					}
				} else { // Zero out the data if the app is not in foreground
					deviceData.fAxes.assign(6, 0.f);
				}
			} else if (pRawInput->data.hid.bRawData[0] == 0x02) { // Rotation vector
				// If we are not in foreground do nothing
				// The rotation vector was zeroed out with the translation vector in the previous message
				if (bIsForeground) {
					TInputData& deviceData = fDevice2Data[pRawInput->header.hDevice];
					deviceData.fTimeToLive = kTimeToLive;

					short* pnRawData = reinterpret_cast<short*>(&pRawInput->data.hid.bRawData[1]);
					// Cache the rotation data
					deviceData.fAxes[3] = static_cast<float>(pnRawData[0]);
					deviceData.fAxes[4] = static_cast<float>(pnRawData[1]);
					deviceData.fAxes[5] = static_cast<float>(pnRawData[2]);
					deviceData.fIsDirty = true;

#if _TRACE_RI_RAWDATA
					qDebug("Rotation RI Data =\t0x%x,\t0x%x,\t0x%x\n",
						pnRawData[0],  pnRawData[1], pnRawData[2]);
#endif
					return true;
				}
			} else if (pRawInput->data.hid.bRawData[0] == 0x03)  { // Keystate change
				// this is a package that contains 3d mouse keystate information
				// bit0=key1, bit=key2 etc.


				unsigned long dwKeystate = *reinterpret_cast<unsigned long*>(&pRawInput->data.hid.bRawData[1]);
#if _TRACE_RI_RAWDATA
				qDebug("ButtonData =0x%x\n", dwKeystate);
#endif
				// Log the keystate changes
				unsigned long dwOldKeystate = fDevice2Keystate[pRawInput->header.hDevice];
				if (dwKeystate != 0) {
					fDevice2Keystate[pRawInput->header.hDevice] = dwKeystate;
				} else {
					fDevice2Keystate.erase(pRawInput->header.hDevice);
				}

				//  Only call the keystate change handlers if the app is in foreground
				if (bIsForeground) {
					unsigned long dwChange = dwKeystate ^ dwOldKeystate;


					for (int nKeycode=1; nKeycode<33; nKeycode++) {
						if (dwChange & 0x01) {
							int nVirtualKeyCode = HidToVirtualKey(sRidDeviceInfo.hid.dwProductId, nKeycode);
							if (nVirtualKeyCode) {
								if (dwKeystate&0x01) {
									On3dmouseKeyDown(pRawInput->header.hDevice, nVirtualKeyCode);
								} else {
									On3dmouseKeyUp(pRawInput->header.hDevice, nVirtualKeyCode);
								}
							}
						}
						dwChange >>=1;
						dwKeystate >>=1;
					}
				}
			}
            else {
                // SpaceMouse Plus XT
                return ParseRawInput(nInputCode, pRawInput);
            }
		}
   }
   return false;
}

// ----------------------------------------------------------------------------

// SpaceMouse Plus XT

// https://www.codeproject.com/Articles/185522/Using-the-Raw-Input-API-to-Process-Joystick-Input
// https://zfx.info/viewtopic.php?f=11&t=2977
//

#include <hidsdi.h>
#include <hidpi.h>
#include <hidusage.h>

//#pragma comment(lib, "user32.lib")
//#pragma comment(lib, "hid.lib")

#define MAX_BUTTONS		128
#define CHECK(exp)		{ if(!(exp)) goto Error; }
#define SAFE_FREE(p)	{ if(p) { HeapFree(hHeap, 0, p); (p) = NULL; } }

bool Gui::GuiNativeEvent::ParseRawInput(UINT nInputCode, PRAWINPUT pRawInput)
{
    bool processed = false;
    bool bIsForeground = (nInputCode == RIM_INPUT);

    PHIDP_PREPARSED_DATA pPreparsedData;
    HIDP_CAPS            Caps;
    PHIDP_BUTTON_CAPS    pButtonCaps;
    PHIDP_VALUE_CAPS     pValueCaps;
    USHORT               capsLength;
    UINT                 bufferSize;
    HANDLE               hHeap;
    USAGE                usage[MAX_BUTTONS];
    ULONG                i, usageLength, value;

    BOOL bButtonStates[MAX_BUTTONS];

    pPreparsedData = NULL;
    pButtonCaps    = NULL;
    pValueCaps     = NULL;
    hHeap          = GetProcessHeap();

    //
    // Get the preparsed data block
    //

    CHECK( GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize) == 0 );
    CHECK( pPreparsedData = (PHIDP_PREPARSED_DATA)HeapAlloc(hHeap, 0, bufferSize) );
    CHECK( (int)GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &bufferSize) >= 0 );

    //
    // Get the joystick's capabilities
    //

    // Button caps
    CHECK( HidP_GetCaps(pPreparsedData, &Caps) == HIDP_STATUS_SUCCESS )
    CHECK( pButtonCaps = (PHIDP_BUTTON_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_BUTTON_CAPS) * Caps.NumberInputButtonCaps) );

    capsLength = Caps.NumberInputButtonCaps;
    CHECK( HidP_GetButtonCaps(HidP_Input, pButtonCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS )
    usageLength = pButtonCaps->Range.UsageMax - pButtonCaps->Range.UsageMin + 1;

    // Value caps
    CHECK( pValueCaps = (PHIDP_VALUE_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps) );
    capsLength = Caps.NumberInputValueCaps;
    CHECK( HidP_GetValueCaps(HidP_Input, pValueCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS )

    processed = true;

    //
    // Get the pressed buttons
    //

    CHECK(
        HidP_GetUsages(
            HidP_Input, pButtonCaps->UsagePage, 0, usage, &usageLength, pPreparsedData,
            (PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
        ) == HIDP_STATUS_SUCCESS );

    ZeroMemory(bButtonStates, sizeof(bButtonStates));
    for(i = 0; i < usageLength; i++)
        bButtonStates[usage[i] - pButtonCaps->Range.UsageMin] = TRUE;

    //
    // Get the state of discrete-valued-controls
    //

    TInputData& deviceData = fDevice2Data[pRawInput->header.hDevice];
    deviceData.fTimeToLive = kTimeToLive;
    if (bIsForeground) {
        for(i = 0; i < Caps.NumberInputValueCaps; i++)
        {
            HidP_GetUsageValue(
                HidP_Input, pValueCaps[i].UsagePage, 0, pValueCaps[i].Range.UsageMin, &value, pPreparsedData,
                (PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
            );

            short svalue = static_cast<short>(value);
            switch(pValueCaps[i].Range.UsageMin)
            {
            case HID_USAGE_GENERIC_X:	// X-axis
                qDebug("X-Axis: %d\n", svalue);
                deviceData.fAxes[0] = static_cast<float>(svalue);
                deviceData.fIsDirty = true;
                break;

            case HID_USAGE_GENERIC_Y:	// Y-axis
                qDebug("Y-Axis: %d\n", svalue);
                deviceData.fAxes[1] = static_cast<float>(svalue);
                deviceData.fIsDirty = true;
                break;

            case HID_USAGE_GENERIC_Z: // Z-axis
                qDebug("Z-Axis: %d\n", svalue);
                deviceData.fAxes[2] = static_cast<float>(svalue);
                deviceData.fIsDirty = true;
                break;

            case HID_USAGE_GENERIC_RX: // Rotate-X
                qDebug("X-Rotate: %d\n", svalue);
                deviceData.fAxes[3] = static_cast<float>(svalue);
                deviceData.fIsDirty = true;
                break;

            case HID_USAGE_GENERIC_RY: // Rotate-Y
                qDebug("Y-Rotate: %d\n", svalue);
                deviceData.fAxes[4] = static_cast<float>(svalue);
                deviceData.fIsDirty = true;
                break;

            case HID_USAGE_GENERIC_RZ: // Rotate-Z
                qDebug("Z-Rotate: %d\n", svalue);
                deviceData.fAxes[5] = static_cast<float>(svalue);
                deviceData.fIsDirty = true;
                break;

            default:
                break;
            }
        }
    }
    else {
        // Zero out the data if the app is not in foreground
        deviceData.fAxes.assign(6, 0.f);
        deviceData.fIsDirty = true;
        qDebug("Not in foreground\n");
    }

    //
    // Clean up
    //

Error:
    SAFE_FREE(pPreparsedData);
    SAFE_FREE(pButtonCaps);
    SAFE_FREE(pValueCaps);

    return processed;
}

#include "3Dconnexion/moc_GuiNativeEventWin32.cpp"
