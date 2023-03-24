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
With special thanks to marcxs for making the first steps
 */

#include <FCConfig.h>
#include "GuiNativeEventMac.h"

#include <unistd.h>
#include "GuiApplicationNativeEventAware.h"
#include <Base/Console.h>

// Suppress warnings to kConnexionMsgDeviceState and tdxAppID
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wfour-char-constants"
#endif

UInt16 Gui::GuiNativeEvent::tdxClientID = 0;
uint32_t Gui::GuiNativeEvent::lastButtons = 0;

/* ----------------------------------------------------------------------------
     Handler for driver events. This function is able to handle the events in
     different ways: (1) re-package the events as Carbon events, (2) compute
     them directly, (3) write the event info in a shared memory location for
     usage by reader threads.
*/
void
Gui::GuiNativeEvent::tdx_drv_handler(io_connect_t connection,
                                     natural_t messageType,
                                     void *messageArgument)
{
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        Base::Console().Log("Call connexion handler\n");
    }
    Q_UNUSED(connection)
    //printf("tdx_drv_handler\n");
    //printf("connection: %X\n", connection);
    //printf("messageType %c%c%c%c\n", messageType/0x1000000, messageType/0x10000, messageType/0x100, messageType);
    auto msg = (ConnexionDeviceStatePtr)messageArgument;

    switch(messageType) {
    case kConnexionMsgDeviceState:
        /* Device state messages are broadcast to all clients.  It is up to
         * the client to figure out if the message is meant for them. This
         * is done by comparing the "client" id sent in the message to our
         * assigned id when the connection to the driver was established.
         */
        //printf("msg->client: %d, tdxClientID: %d\n", msg->client, tdxClientID);
        Base::Console().Log("msg->client: %d, msg->command: %d\n", msg->client, msg->command);

        if (msg->client == tdxClientID) {
            switch (msg->command) {
            case kConnexionCmdHandleAxis:
            {
                motionDataArray[0] = -msg->axis[0];
                motionDataArray[1] = msg->axis[1];
                motionDataArray[2] = msg->axis[2];
                motionDataArray[3] = -msg->axis[3];
                motionDataArray[4] = msg->axis[4];
                motionDataArray[5] = msg->axis[5];
                mainApp->postMotionEvent(motionDataArray);
                break;
            }

            case kConnexionCmdHandleButtons:
            {
                //printf("value: %d\n", msg->value);
                //printf("buttons: %u\n", msg->buttons);
                uint32_t changedButtons = msg->buttons ^ lastButtons;
                uint32_t pressedButtons = msg->buttons & changedButtons;
                uint32_t releasedButtons = lastButtons & changedButtons;
                for (uint8_t bt = 0; bt < 32; bt++) {
                    if (pressedButtons & 1)
                        mainApp->postButtonEvent(bt, 1);
                    pressedButtons = pressedButtons >> 1;
                }
                for (uint8_t bt = 0; bt < 32; bt++) {
                    if (releasedButtons & 1)
                        mainApp->postButtonEvent(bt, 0);
                    releasedButtons = releasedButtons >> 1;
                }

                lastButtons = msg->buttons;
                break;
            }

            default:
                break;

            } /* switch */
        }
        break;

    default:
        /* other messageTypes can happen and should be ignored */
        break;
    }
}

Gui::GuiNativeEvent::GuiNativeEvent(Gui::GUIApplicationNativeEventAware *app)
: GuiAbstractNativeEvent(app)
{
}

Gui::GuiNativeEvent::~GuiNativeEvent()
{
    // if 3Dconnexion library was loaded at runtime
    if (InstallConnexionHandlers) {
        // Close our connection with the 3dx driver
        if (tdxClientID)
            UnregisterConnexionClient(tdxClientID);
        CleanupConnexionHandlers();
        Base::Console().Log("Disconnected from 3Dconnexion driver\n");
    }
}

void Gui::GuiNativeEvent::initSpaceball(QMainWindow *window)
{
    Q_UNUSED(window)
    OSStatus err;
    /* make sure the framework is installed */
    if (SetConnexionHandlers == NULL)
    {
        Base::Console().Log("3Dconnexion framework not found!\n");
        return;
    }
    /* install 3dx message handler in order to receive driver events */
    err = SetConnexionHandlers(tdx_drv_handler, 0L, 0L, false);
    assert(err == 0);
    if (err)
    {
        Base::Console().Log("Error installing 3Dconnexion handler\n");
        return;
    }
    /* register our app with the driver */
    // Pascal string Application name required to register driver for application
    UInt8  tdxAppName[] = {7,'F','r','e','e','C','A','D'};
    // 32bit appID to register driver for application
    UInt32 tdxAppID = 'FCAd';
    tdxClientID = RegisterConnexionClient( tdxAppID, tdxAppName,
                                           kConnexionClientModeTakeOver,
                                           kConnexionMaskAll );
    if (tdxClientID == 0)
    {
        Base::Console().Log("Couldn't connect to 3Dconnexion driver\n");
        return;
    }

    // Turn on all features and buttons
    SetConnexionClientMask(tdxClientID, kConnexionMaskAll);
    SetConnexionClientButtonMask(tdxClientID, kConnexionMaskAllButtons);

    Base::Console().Log("3Dconnexion driver initialized. Client ID: %d\n", tdxClientID);
    mainApp->setSpaceballPresent(true);
}

#include "3Dconnexion/moc_GuiNativeEventMac.cpp"
