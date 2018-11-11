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

#ifndef GUINATIVEEVENT_H
#define GUINATIVEEVENT_H

#include <QObject>

#include "3Dconnexion/MouseParameters.h"

#include <vector>
#include <map>

//#define _WIN32_WINNT 0x0501  //target at least windows XP
#include <Windows.h>
#include <QAbstractNativeEventFilter>


class QMainWindow;
class GUIApplicationNativeEventAware;

namespace Gui
{
#if QT_VERSION >= 0x050000
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
#endif // if QT_VERSION >= 0x050000
	class GUIApplicationNativeEventAware;

	class GuiNativeEvent : public QObject
	{
#include "GuiNativeEventCommon.h"
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
        static Gui::GuiNativeEvent* gMouseInput;
	};
}

#endif //GUINATIVEEVENT_H

