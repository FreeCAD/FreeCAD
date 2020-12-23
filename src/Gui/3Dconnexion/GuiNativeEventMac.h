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

#include "GuiAbstractNativeEvent.h"

class QMainWindow;

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

namespace Gui
{
	class GUIApplicationNativeEventAware;

	class GuiNativeEvent : public GuiAbstractNativeEvent
	{
	Q_OBJECT
	public:
		GuiNativeEvent(GUIApplicationNativeEventAware *app);
		~GuiNativeEvent() override final;
		void initSpaceball(QMainWindow *window) override final;
	private:
		GuiNativeEvent();
		GuiNativeEvent(const GuiNativeEvent&);
		GuiNativeEvent& operator=(const GuiNativeEvent&);
	private:
        static UInt16 tdxClientID; /* ID assigned by the driver */
        static uint32_t lastButtons;
        static void tdx_drv_handler( io_connect_t connection,
                                     natural_t messageType,
                                     void *messageArgument );
	};
}

#endif //GUINATIVEEVENT_H

