/***************************************************************************
 *   Copyright (c) 2010 Thomas Anderson <ta@nextgenengineering>            *
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



#ifndef GUIAPPLICATIONNATIVEEVENTAWARE_H
#define GUIAPPLICATIONNATIVEEVENTAWARE_H

#include <QApplication>

class QMainWindow;

#if defined(Q_OS_WIN) && defined(_USE_3DCONNEXION_SDK)
#include "3Dconnexion/MouseParameters.h"

#include <vector>
#include <map>

//#define _WIN32_WINNT 0x0501  //target at least windows XP
#include <Windows.h>
#include <QAbstractNativeEventFilter>

#elif defined(Q_OS_MACX) && defined(_USE_3DCONNEXION_SDK)

#include <IOKit/IOKitLib.h>
#include <ConnexionClientAPI.h>
// Note that InstallConnexionHandlers will be replaced with
// SetConnexionHandlers "in the future".
extern OSErr InstallConnexionHandlers(ConnexionMessageHandlerProc messageHandler,
                                      ConnexionAddedHandlerProc addedHandler,
                                      ConnexionRemovedHandlerProc removedHandler)
                                      __attribute__((weak_import));
extern UInt16 RegisterConnexionClient(UInt32 signature, UInt8 *name, UInt16 mode,
                                      UInt32 mask) __attribute__((weak_import));
extern void UnregisterConnexionClient(UInt16 clientID) __attribute__((weak_import));
extern void CleanupConnexionHandlers(void) __attribute__((weak_import));
#endif // Platform switch

namespace Gui
{
    class GUIApplicationNativeEventAware : public QApplication
    {
        Q_OBJECT
    public:
        GUIApplicationNativeEventAware(int &argc, char *argv[]);
        ~GUIApplicationNativeEventAware();
        void initSpaceball(QMainWindow *window);
        bool isSpaceballPresent() const {return spaceballPresent;}
        bool processSpaceballEvent(QObject *object, QEvent *event);
    private:
        bool spaceballPresent;
        QMainWindow *mainWindow;
        int  motionDataArray[6];
        bool setOSIndependentMotionData();
        void importSettings();
        float convertPrefToSensitivity(int value);

#ifdef Q_OS_LINUX
    public:
        void pollSpacenav();
#endif // Q_OS_LINUX

#ifdef _USE_3DCONNEXION_SDK
// For Windows
#ifdef Q_OS_WIN
    class RawInputEventFilter : public QAbstractNativeEventFilter
    {
    public:
        typedef bool (*EventFilter)(void *message, long *result);
        RawInputEventFilter(EventFilter filter) : eventFilter(filter) {
        }
        virtual ~RawInputEventFilter() {
        }

        virtual bool nativeEventFilter(const QByteArray & /*eventType*/, void *message, long *result) {
            return eventFilter(message, result);
        }

    private:
        EventFilter eventFilter;
    };

    public:
        static bool Is3dmouseAttached();

        I3dMouseParam& MouseParams();
        const I3dMouseParam& MouseParams() const;

        virtual void Move3d(HANDLE device, std::vector<float>& motionData);
        virtual void On3dmouseKeyDown(HANDLE device, int virtualKeyCode);
        virtual void On3dmouseKeyUp(HANDLE device, int virtualKeyCode);

    private:
        bool InitializeRawInput(HWND hwndTarget);
        static bool RawInputEventFilter(void* msg, long* result);
        void OnRawInput(UINT nInputCode, HRAWINPUT hRawInput);
        UINT GetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader);
        bool TranslateRawInputData(UINT nInputCode, PRAWINPUT pRawInput);
        bool ParseRawInput(UINT nInputCode, PRAWINPUT pRawInput);
        void On3dmouseInput();

        class TInputData
        {
        public:
            TInputData() : fAxes(6) {}

            bool IsZero() {
                return (0.==fAxes[0] && 0.==fAxes[1] && 0.==fAxes[2] &&
                        0.==fAxes[3] && 0.==fAxes[4] && 0.==fAxes[5]);
            }

            int fTimeToLive; // For telling if the device was unplugged while sending data
            bool fIsDirty;
            std::vector<float>     fAxes;
        };

        HWND fWindow;

        // Data cache to handle multiple rawinput devices
        std::map< HANDLE, TInputData>       fDevice2Data;
        std::map< HANDLE, unsigned long>    fDevice2Keystate;
        // 3dmouse parameters
        MouseParameters f3dMouseParams;     // Rotate, Pan Zoom etc.
        // use to calculate distance traveled since last event
        DWORD fLast3dmouseInputTime;
        static Gui::GUIApplicationNativeEventAware* gMouseInput;
#endif // Q_OS_WIN
#ifdef Q_OS_MACX
    private:
        static UInt16 tdxClientID; /* ID assigned by the driver */
        static uint32_t lastButtons;
    public:
        static void tdx_drv_handler( io_connect_t connection,
                                     natural_t messageType,
                                     void *messageArgument );
        void Move3d();
        void Button3d(bool buttonDown, int buttonNumber);

#endif// Q_OS_MACX
#endif // _USE_3DCONNEXION_SDK
    }; // end class GUIApplicationNativeEventAware
} // end namespace Gui

#endif // GUIAPPLICATIONNATIVEEVENTAWARE_H
