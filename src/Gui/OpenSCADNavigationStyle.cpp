/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/nodes/SoCamera.h>
# include <QApplication>
#endif

#include "NavigationStyle.h"
#include "View3DInventorViewer.h"


using namespace Gui;

// ----------------------------------------------------------------------------------

/* TRANSLATOR Gui::OpenSCADNavigationStyle */

TYPESYSTEM_SOURCE(Gui::OpenSCADNavigationStyle, Gui::UserNavigationStyle)

OpenSCADNavigationStyle::OpenSCADNavigationStyle() = default;

OpenSCADNavigationStyle::~OpenSCADNavigationStyle() = default;

const char* OpenSCADNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
    case NavigationStyle::SELECTION:
        return QT_TR_NOOP("Press left mouse button");
    case NavigationStyle::PANNING:
        return QT_TR_NOOP("Press right mouse button and move mouse");
    case NavigationStyle::DRAGGING:
        return QT_TR_NOOP("Press left mouse button and move mouse");
    case NavigationStyle::ZOOMING:
        return QT_TR_NOOP("Press middle mouse button or SHIFT and right mouse button");
    default:
        return "No description";
    }
}

SbBool OpenSCADNavigationStyle::processSoEvent(const SoEvent * const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) {
        return inherited::processSoEvent(ev);
    }
    // Switch off viewing mode
    if (!this->isSeekMode() && !this->isAnimating() && this->isViewing())
        this->setViewing(false); // by default disable viewing mode to render the scene

    const SoType type(ev->getTypeId());

    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    const SbVec2s pos(ev->getPosition());
    const SbVec2f posn = normalizePixelPos(pos);

    const SbVec2f prevnormalized = this->lastmouseposition;
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
    syncModifierKeys(ev);

    // give the nodes in the foreground root the chance to handle events (e.g color bar)
    if (!viewer->isEditing()) {
        processed = handleEventInForeground(ev);
        if (processed)
            return true;
    }

    // Keyboard handling
    if (type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const auto event = static_cast<const SoKeyboardEvent *>(ev);
        processed = processKeyboardEvent(event);
    }

    // Mouse Button / Spaceball Button handling
    if (type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const auto event = (const SoMouseButtonEvent *) ev;
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

        switch (button) {
        case SoMouseButtonEvent::BUTTON1:
            this->lockrecenter = true;
            this->button1down = press;
            if (press && (curmode == NavigationStyle::SEEK_WAIT_MODE)) {
                newmode = NavigationStyle::SEEK_MODE;
                this->seekToPoint(pos); // implicitly calls interactiveCountInc()
                processed = true;
            }
            else if (!press && (curmode == NavigationStyle::ZOOMING)) {
                newmode = NavigationStyle::IDLE;
                processed = true;
            }
            else if (!press && (curmode == NavigationStyle::DRAGGING)) {
                this->setViewing(false);
                processed = true;
            }
            else if (viewer->isEditing() && (curmode == NavigationStyle::SPINNING)) {
                processed = true;
            }
            else {
                processed = processClickEvent(event);
            }
            break;
        case SoMouseButtonEvent::BUTTON2:
            // If we are in edit mode then simply ignore the RMB events
            // to pass the event to the base class.
            this->lockrecenter = true;
            this->button2down = press;
            if (!viewer->isEditing()) {
                // If we are in zoom or pan mode ignore RMB events otherwise
                // the canvas doesn't get any release events
                if (curmode != NavigationStyle::ZOOMING &&
                    curmode != NavigationStyle::PANNING &&
                    curmode != NavigationStyle::DRAGGING) {
                    if (this->isPopupMenuEnabled()) {
                        if (!press) { // release right mouse button
                            this->openPopupMenu(event->getPosition());
                        }
                    }
                }
            }
            // Alternative way of rotating & zooming
            if (press && (curmode == NavigationStyle::PANNING ||
                          curmode == NavigationStyle::ZOOMING)) {
                newmode = NavigationStyle::DRAGGING;
                saveCursorPosition(ev);
                this->centerTime = ev->getTime();
                processed = true;
            }
            else if (!press && (curmode == NavigationStyle::DRAGGING)) {
                newmode = NavigationStyle::IDLE;
                processed = true;
            }
            break;
        case SoMouseButtonEvent::BUTTON3:
            this->button3down = press;
            if (press) {
                this->centerTime = ev->getTime();
                float ratio = vp.getViewportAspectRatio();
                SbViewVolume vv = viewer->getSoRenderManager()->getCamera()->getViewVolume(ratio);
                this->panningplane = vv.getPlane(viewer->getSoRenderManager()->getCamera()->focalDistance.getValue());
                this->lockrecenter = false;
            }
            else if (curmode == NavigationStyle::PANNING) {
                newmode = NavigationStyle::IDLE;
                processed = true;
            }
            break;
        default:
            break;
        }
    }

    // Mouse Movement handling
    if (type.isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        this->lockrecenter = true;
        const auto event = (const SoLocation2Event *) ev;
        if (curmode == NavigationStyle::SELECTION) {
            newmode = NavigationStyle::DRAGGING;
            saveCursorPosition(ev);
            this->centerTime = ev->getTime();
        }
        else if (curmode == NavigationStyle::ZOOMING) {
            // OpenSCAD uses vertical mouse position, not horizontal
            // this->zoomByCursor(posn, prevnormalized);
            float value = (posn[1] - prevnormalized[1]) * 10.0f;
            if (this->invertZoom)
                value = -value;
            zoom(viewer->getSoRenderManager()->getCamera(), value);
            processed = true;
        }
        else if (curmode == NavigationStyle::PANNING) {
            float ratio = vp.getViewportAspectRatio();
            panCamera(viewer->getSoRenderManager()->getCamera(), ratio, this->panningplane, posn, prevnormalized);
            processed = true;
        }
        else if (curmode == NavigationStyle::DRAGGING) {
            this->addToLog(event->getPosition(), event->getTime());
            this->spin(posn);
            moveCursorPosition();
            processed = true;
        }
    }

    // Spaceball & Joystick handling
    if (type.isDerivedFrom(SoMotion3Event::getClassTypeId())) {
        const auto event = static_cast<const SoMotion3Event *>(ev);
        if (event)
            this->processMotionEvent(event);
        processed = true;
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

    switch (combo) {
    case 0:
        if (curmode == NavigationStyle::SPINNING) { break; }
        newmode = NavigationStyle::IDLE;
        break;
    case BUTTON1DOWN:
        if (newmode != NavigationStyle::DRAGGING)
            newmode = NavigationStyle::SELECTION;
        break;
    case BUTTON2DOWN:
        newmode = NavigationStyle::PANNING;
        break;
    case BUTTON3DOWN:
    case SHIFTDOWN|BUTTON2DOWN:
    case SHIFTDOWN|BUTTON3DOWN:
        newmode = NavigationStyle::ZOOMING;
        break;
    default:
        break;
    }

    if (newmode != curmode) {
        this->setViewingMode(newmode);
    }

    // If not handled in this class, pass on upwards in the inheritance
    // hierarchy.
    if (!processed)
        processed = inherited::processSoEvent(ev);
    return processed;
}
