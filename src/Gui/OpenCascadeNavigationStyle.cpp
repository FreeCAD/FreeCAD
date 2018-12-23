/***************************************************************************
 *   Copyright (c) 2015 Kirill Gavrilov <kirill.gavrilov[at]opencascade.com> *
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
#ifndef _PreComp_
# include <cfloat>
# include "InventorAll.h"
# include <QAction>
# include <QActionGroup>
# include <QApplication>
# include <QByteArray>
# include <QCursor>
# include <QList>
# include <QMenu>
# include <QMetaObject>
# include <QRegExp>
#endif

#include <Inventor/sensors/SoTimerSensor.h>

#include <App/Application.h>
#include "NavigationStyle.h"
#include "View3DInventorViewer.h"
#include "Application.h"
#include "MenuManager.h"
#include "MouseSelection.h"

using namespace Gui;

// ----------------------------------------------------------------------------------

/* TRANSLATOR Gui::OpenCascadeNavigationStyle */

TYPESYSTEM_SOURCE(Gui::OpenCascadeNavigationStyle, Gui::UserNavigationStyle);

OpenCascadeNavigationStyle::OpenCascadeNavigationStyle()
{
}

OpenCascadeNavigationStyle::~OpenCascadeNavigationStyle()
{
}

const char* OpenCascadeNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
    case NavigationStyle::SELECTION:
        return QT_TR_NOOP("Press left mouse button");
    case NavigationStyle::PANNING:
        return QT_TR_NOOP("Press CTRL and middle mouse button");
    case NavigationStyle::DRAGGING:
        return QT_TR_NOOP("Press CTRL and right mouse button");
    case NavigationStyle::ZOOMING:
        return QT_TR_NOOP("Press CTRL and left mouse button");
    default:
        return "No description";
    }
}

SbBool OpenCascadeNavigationStyle::processSoEvent(const SoEvent * const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) { return inherited::processSoEvent(ev); }
    // Switch off viewing mode
    if (!this->isSeekMode() && !this->isAnimating() && this->isViewing())
        this->setViewing(false); // by default disable viewing mode to render the scene

    const SoType type(ev->getTypeId());

    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    const SbVec2s size(vp.getViewportSizePixels());
    const SbVec2f prevnormalized = this->lastmouseposition;
    const SbVec2s pos(ev->getPosition());
    const SbVec2f posn((float) pos[0] / (float) std::max((int)(size[0] - 1), 1),
                       (float) pos[1] / (float) std::max((int)(size[1] - 1), 1));

    this->lastmouseposition = posn;

    // Set to true if any event processing happened. Note that it is not
    // necessary to restrict ourselves to only do one "action" for an
    // event, we only need this flag to see if any processing happened
    // at all.
    SbBool processed = false;

    const ViewerMode curmode = this->currentmode;
    ViewerMode newmode = curmode;

    // Mismatches in state of the modifier keys happens if the user
    // presses or releases them outside the viewer window.
    if (this->ctrldown != ev->wasCtrlDown()) {
        this->ctrldown = ev->wasCtrlDown();
    }
    if (this->shiftdown != ev->wasShiftDown()) {
        this->shiftdown = ev->wasShiftDown();
    }
    if (this->altdown != ev->wasAltDown()) {
        this->altdown = ev->wasAltDown();
    }

    // give the nodes in the foreground root the chance to handle events (e.g color bar)
    if (!processed && !viewer->isEditing()) {
        processed = handleEventInForeground(ev);
        if (processed)
            return true;
    }

    // Keyboard handling
    if (type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const SoKeyboardEvent * const event = (const SoKeyboardEvent *) ev;
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;
        switch (event->getKey()) {
        case SoKeyboardEvent::LEFT_CONTROL:
        case SoKeyboardEvent::RIGHT_CONTROL:
            this->ctrldown = press;
            break;
        case SoKeyboardEvent::LEFT_SHIFT:
        case SoKeyboardEvent::RIGHT_SHIFT:
            this->shiftdown = press;
            break;
        case SoKeyboardEvent::LEFT_ALT:
        case SoKeyboardEvent::RIGHT_ALT:
            this->altdown = press;
            break;
        case SoKeyboardEvent::H:
            processed = true;
            viewer->saveHomePosition();
            break;
        case SoKeyboardEvent::S:
        case SoKeyboardEvent::HOME:
        case SoKeyboardEvent::LEFT_ARROW:
        case SoKeyboardEvent::UP_ARROW:
        case SoKeyboardEvent::RIGHT_ARROW:
        case SoKeyboardEvent::DOWN_ARROW:
            if (!this->isViewing())
                this->setViewing(true);
            break;
        default:
            break;
        }
    }

    // Mouse Button / Spaceball Button handling
    if (type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent * const event = (const SoMouseButtonEvent *) ev;
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

        switch (button) {
        case SoMouseButtonEvent::BUTTON1:
            this->lockrecenter = true;
            this->button1down = press;
            if (press && (this->currentmode == NavigationStyle::SEEK_WAIT_MODE)) {
                newmode = NavigationStyle::SEEK_MODE;
                this->seekToPoint(pos); // implicitly calls interactiveCountInc()
                processed = true;
            }
            else if (!press && (this->currentmode == NavigationStyle::ZOOMING)) {
                newmode = NavigationStyle::IDLE;
                processed = true;
            }
            else if (!press && (this->currentmode == NavigationStyle::DRAGGING)) {
                this->setViewing(false);
                processed = true;
            }
            else if (viewer->isEditing() && (this->currentmode == NavigationStyle::SPINNING)) {
                processed = true;
            }
            // issue #0002433: avoid to swallow the UP event if down the
            // scene graph somewhere a dialog gets opened
            else if (press) {
                SbTime tmp = (ev->getTime() - mouseDownConsumedEvent.getTime());
                float dci = (float)QApplication::doubleClickInterval()/1000.0f;
                // a double-click?
                if (tmp.getValue() < dci) {
                    mouseDownConsumedEvent = *event;
                    mouseDownConsumedEvent.setTime(ev->getTime());
                    processed = true;
                }
                else {
                    mouseDownConsumedEvent.setTime(ev->getTime());
                    // 'ANY' is used to mark that we don't know yet if it will
                    // be a double-click event.
                    mouseDownConsumedEvent.setButton(SoMouseButtonEvent::ANY);
                }
            }
            else if (!press) {
                if (mouseDownConsumedEvent.getButton() == SoMouseButtonEvent::BUTTON1) {
                    // now handle the postponed event
                    inherited::processSoEvent(&mouseDownConsumedEvent);
                    mouseDownConsumedEvent.setButton(SoMouseButtonEvent::ANY);
                }
            }
            break;
        case SoMouseButtonEvent::BUTTON2:
            // If we are in edit mode then simply ignore the RMB events
            // to pass the event to the base class.
            this->lockrecenter = true;
            if (!viewer->isEditing()) {
                // If we are in zoom or pan mode ignore RMB events otherwise
                // the canvas doesn't get any release events
                if (this->currentmode != NavigationStyle::ZOOMING &&
                    this->currentmode != NavigationStyle::PANNING &&
                    this->currentmode != NavigationStyle::DRAGGING) {
                    if (this->isPopupMenuEnabled()) {
                        if (!press) { // release right mouse button
                            this->openPopupMenu(event->getPosition());
                        }
                    }
                }
            }
            // Alternative way of rotating & zooming
            if (press && (this->currentmode == NavigationStyle::PANNING ||
                          this->currentmode == NavigationStyle::ZOOMING)) {
                newmode = NavigationStyle::DRAGGING;
                saveCursorPosition(ev);
                this->centerTime = ev->getTime();
                processed = true;
            }
            else if (!press && (this->currentmode == NavigationStyle::DRAGGING)) {
                newmode = NavigationStyle::IDLE;
                processed = true;
            }
            this->button2down = press;
            break;
        case SoMouseButtonEvent::BUTTON3:
            if (press) {
                this->centerTime = ev->getTime();
                float ratio = vp.getViewportAspectRatio();
                SbViewVolume vv = viewer->getSoRenderManager()->getCamera()->getViewVolume(ratio);
                this->panningplane = vv.getPlane(viewer->getSoRenderManager()->getCamera()->focalDistance.getValue());
                this->lockrecenter = false;
            }
            else if (!press && (this->currentmode == NavigationStyle::PANNING)) {
                newmode = NavigationStyle::IDLE;
                processed = true;
            }
            this->button3down = press;
            break;
        case SoMouseButtonEvent::BUTTON4:
        case SoMouseButtonEvent::BUTTON5:
            doZoom(viewer->getSoRenderManager()->getCamera(), button == SoMouseButtonEvent::BUTTON4, posn);
            processed = true;
            break;
        default:
            break;
        }
    }

    enum {
        BUTTON1DOWN = 1 << 0,
        BUTTON3DOWN = 1 << 1,
        CTRLDOWN =    1 << 2,
        SHIFTDOWN =   1 << 3,
        BUTTON2DOWN = 1 << 4
    };
    unsigned int combo =
        (this->button1down ? BUTTON1DOWN : 0) |
        (this->button2down ? BUTTON2DOWN : 0) |
        (this->button3down ? BUTTON3DOWN : 0) |
        (this->ctrldown ? CTRLDOWN : 0) |
        (this->shiftdown ? SHIFTDOWN : 0);

    // Mouse Movement handling
    if (type.isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        this->lockrecenter = true;
        const SoLocation2Event * const event = (const SoLocation2Event *) ev;
        if (this->currentmode == NavigationStyle::ZOOMING) {
            // OCCT uses horizontal mouse position, not vertical
            // this->zoomByCursor(posn, prevnormalized);
            float value = (posn[0] - prevnormalized[0]) * 10.0f;
            if (this->invertZoom)
                value = -value;
            zoom(viewer->getSoRenderManager()->getCamera(), value);
            processed = true;
        }
        else if (this->currentmode == NavigationStyle::PANNING) {
            float ratio = vp.getViewportAspectRatio();
            panCamera(viewer->getSoRenderManager()->getCamera(), ratio, this->panningplane, posn, prevnormalized);
            processed = true;
        }
        else if (this->currentmode == NavigationStyle::DRAGGING) {
            this->addToLog(event->getPosition(), event->getTime());
            this->spin(posn);
            moveCursorPosition();
            processed = true;
        }
        else if (combo == (CTRLDOWN|BUTTON1DOWN)) {
            newmode = NavigationStyle::ZOOMING;
        }
    }

    // Spaceball & Joystick handling
    if (type.isDerivedFrom(SoMotion3Event::getClassTypeId())) {
        const SoMotion3Event * const event = static_cast<const SoMotion3Event *>(ev);
        if (event)
            this->processMotionEvent(event);
        processed = true;
    }

    switch (combo) {
    case 0:
        if (curmode == NavigationStyle::SPINNING) { break; }
        newmode = NavigationStyle::IDLE;
        break;
    case CTRLDOWN|BUTTON1DOWN:
    case BUTTON1DOWN:
        if (newmode != NavigationStyle::ZOOMING)
            newmode = NavigationStyle::SELECTION;
        break;
    case CTRLDOWN|BUTTON3DOWN:
    case BUTTON3DOWN:
        newmode = NavigationStyle::PANNING;
        break;
    case CTRLDOWN|BUTTON2DOWN:
        newmode = NavigationStyle::DRAGGING;
        break;
    case BUTTON2DOWN:
        newmode = NavigationStyle::IDLE;
        break;
    default:
        break;
    }

    if (newmode != curmode) {
        this->setViewingMode(newmode);
    }

    // If not handled in this class, pass on upwards in the inheritance
    // hierarchy.
    if (/*(curmode == NavigationStyle::SELECTION || viewer->isEditing()) && */!processed)
        processed = inherited::processSoEvent(ev);
    else
        return true;

    return processed;
}
