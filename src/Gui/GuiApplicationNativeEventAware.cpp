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

#include "PreCompiled.h"

#include <QGlobalStatic>
#ifdef Q_WS_X11
#include <QX11Info>
#endif
#include <QMainWindow>
#include <QWidget>
#include <FCConfig.h>
#include <Base/Console.h>
#include "GuiApplicationNativeEventAware.h"
#include "SpaceballEvent.h"

//linux dependency libspnav-dev
#ifdef Q_WS_X11
#ifdef SPNAV_FOUND
#include <spnav.h>
#endif
#endif

#ifdef _USE_3DCONNEXION_SDK
Gui::GUIApplicationNativeEventAware* Gui::GUIApplicationNativeEventAware::gMouseInput = 0;
#endif

Gui::GUIApplicationNativeEventAware::GUIApplicationNativeEventAware(int &argc, char *argv[]) :
        QApplication (argc, argv), spaceballPresent(false)
{
    mainWindow = 0;
}

Gui::GUIApplicationNativeEventAware::~GUIApplicationNativeEventAware()
{
#ifdef SPNAV_FOUND
    if (spnav_close())
        Base::Console().Log("Couldn't disconnect from spacenav daemon\n");
    else
        Base::Console().Log("Disconnected from spacenav daemon\n");
#endif

#ifdef _USE_3DCONNEXION_SDK
    if (gMouseInput == this) {
        gMouseInput = 0;
    }
#endif
}

void Gui::GUIApplicationNativeEventAware::initSpaceball(QMainWindow *window)
{
    mainWindow = window;

#ifdef SPNAV_FOUND
    if (spnav_x11_open(QX11Info::display(), window->winId()) == -1)
        Base::Console().Log("Couldn't connect to spacenav daemon\n");
    else
    {
        Base::Console().Log("Connected to spacenav daemon\n");
        spaceballPresent = true;
    }
#endif

#ifdef _USE_3DCONNEXION_SDK
    spaceballPresent = Is3dmouseAttached();

    if (spaceballPresent) {
        fLast3dmouseInputTime = 0;

        if (InitializeRawInput(mainWindow->winId())){
            gMouseInput = this;
            qApp->setEventFilter(Gui::GUIApplicationNativeEventAware::RawInputEventFilter);
        }
    }
#endif // _USE_3DCONNEXION_SDK

    Spaceball::MotionEvent::MotionEventType = QEvent::registerEventType();
    Spaceball::ButtonEvent::ButtonEventType = QEvent::registerEventType();
}

bool Gui::GUIApplicationNativeEventAware::processSpaceballEvent(QObject *object, QEvent *event)
{
    QApplication::notify(object, event);
    if (event->type() == Spaceball::MotionEvent::MotionEventType)
    {
        Spaceball::MotionEvent *motionEvent = dynamic_cast<Spaceball::MotionEvent*>(event);
        if (!motionEvent)
            return true;
        if (!motionEvent->isHandled())
        {
            //make a new event and post to parent.
            Spaceball::MotionEvent *newEvent = new Spaceball::MotionEvent(*motionEvent);
            postEvent(object->parent(), newEvent);
        }
    }

    if (event->type() == Spaceball::ButtonEvent::ButtonEventType)
    {
        Spaceball::ButtonEvent *buttonEvent = dynamic_cast<Spaceball::ButtonEvent*>(event);
        if (!buttonEvent)
            return true;
        if (!buttonEvent->isHandled())
        {
            //make a new event and post to parent.
            Spaceball::ButtonEvent *newEvent = new Spaceball::ButtonEvent(*buttonEvent);
            postEvent(object->parent(), newEvent);
        }
    }
    return true;
}

#ifdef Q_WS_X11
bool Gui::GUIApplicationNativeEventAware::x11EventFilter(XEvent *event)
{
#ifdef SPNAV_FOUND
    spnav_event navEvent;
    if (!spnav_x11_event(event, &navEvent))
        return false;

    QWidget *currentWidget = this->focusWidget();
    if (!currentWidget)
        currentWidget = mainWindow;

    if (navEvent.type == SPNAV_EVENT_MOTION)
    {
        Spaceball::MotionEvent *motionEvent = new Spaceball::MotionEvent();
        motionEvent->setTranslations(navEvent.motion.x, navEvent.motion.y, navEvent.motion.z);
        motionEvent->setRotations(navEvent.motion.rx, navEvent.motion.ry, navEvent.motion.rz);
        this->postEvent(currentWidget, motionEvent);
        return true;
    }

    if (navEvent.type == SPNAV_EVENT_BUTTON)
    {
        Spaceball::ButtonEvent *buttonEvent = new Spaceball::ButtonEvent();
        buttonEvent->setButtonNumber(navEvent.button.bnum);
        if (navEvent.button.press)
            buttonEvent->setButtonStatus(Spaceball::BUTTON_PRESSED);
        else
            buttonEvent->setButtonStatus(Spaceball::BUTTON_RELEASED);
        this->postEvent(currentWidget, buttonEvent);
        return true;
    }

    Base::Console().Log("Unknown spaceball event\n");
    return true;
#else
    return false;
#endif // SPNAV_FOUND
}
#endif // Q_WS_X11

#include "moc_GuiApplicationNativeEventAware.cpp"
