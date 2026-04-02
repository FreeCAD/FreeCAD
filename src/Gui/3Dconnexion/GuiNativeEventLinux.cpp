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

#include <FCConfig.h>

#include "GuiNativeEventLinux.h"

#include "GuiApplicationNativeEventAware.h"
#include <Base/Console.h>
#include <QMainWindow>

#include <QSocketNotifier>

#include <cerrno>
#include <sys/socket.h>

#include <spnav.h>

Gui::GuiNativeEvent::GuiNativeEvent(Gui::GUIApplicationNativeEventAware* app)
    : GuiAbstractNativeEvent(app)
{}

Gui::GuiNativeEvent::~GuiNativeEvent()
{
    if (spnavNotifier) {
        if (spnav_close()) {
            Base::Console().log("Couldn't disconnect from spacenav daemon\n");
        }
        else {
            Base::Console().log("Disconnected from spacenav daemon\n");
        }
    }
}

void Gui::GuiNativeEvent::initSpaceball(QMainWindow* window)
{
    Q_UNUSED(window)
    if (spnav_open() == -1) {
        Base::Console().log(
            "Couldn't connect to spacenav daemon. Please ignore if you don't have a spacemouse.\n"
        );
    }
    else {
        Base::Console().log("Connected to spacenav daemon\n");
        spnavNotifier = new QSocketNotifier(spnav_fd(), QSocketNotifier::Read, this);
        connect(spnavNotifier, SIGNAL(activated(int)), this, SLOT(pollSpacenav()));
        mainApp->setSpaceballPresent(true);
    }
}

void Gui::GuiNativeEvent::pollSpacenav()
{
    spnav_event ev;
    bool gotEvent = false;

    while (spnav_poll_event(&ev)) {
        gotEvent = true;
        switch (ev.type) {
            case SPNAV_EVENT_MOTION: {
                motionDataArray[0] = -ev.motion.x;
                motionDataArray[1] = -ev.motion.z;
                motionDataArray[2] = -ev.motion.y;
                motionDataArray[3] = -ev.motion.rx;
                motionDataArray[4] = -ev.motion.rz;
                motionDataArray[5] = -ev.motion.ry;
                mainApp->postMotionEvent(motionDataArray);
                break;
            }
            case SPNAV_EVENT_BUTTON: {
                mainApp->postButtonEvent(ev.button.bnum, ev.button.press);
                break;
            }
        }
    }

    if (!gotEvent) {
        // QSocketNotifier fired but no events were available.
        // Verify the connection is still alive using a non-consuming peek.
        int fd = spnav_fd();
        if (fd >= 0) {
            char buf;
            ssize_t ret = recv(fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
            if (ret == 0 || (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                // EOF or socket error — spacenavd disconnected
                Base::Console().warning(
                    "Lost connection to spacenav daemon. Restart FreeCAD to reconnect.\n"
                );
                spnavNotifier->setEnabled(false);
                spnav_close();
                spnavNotifier = nullptr;
            }
        }
    }
}

#include "3Dconnexion/moc_GuiNativeEventLinux.cpp"
