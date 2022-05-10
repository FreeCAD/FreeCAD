/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

/* TRANSLATOR Gui::RevitNavigationStyle */

TYPESYSTEM_SOURCE(Gui::RevitNavigationStyle, Gui::UserNavigationStyle)

RevitNavigationStyle::RevitNavigationStyle() : lockButton1(false)
{
}

RevitNavigationStyle::~RevitNavigationStyle()
{
}

const char* RevitNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
    case NavigationStyle::SELECTION:
        return QT_TR_NOOP("Press left mouse button");
    case NavigationStyle::PANNING:
        return QT_TR_NOOP("Press middle mouse button");
    case NavigationStyle::DRAGGING:
        return QT_TR_NOOP("Press SHIFT and middle mouse button");
    case NavigationStyle::ZOOMING:
        return QT_TR_NOOP("Scroll middle mouse button");
    default:
        return "No description";
    }
}

SbBool RevitNavigationStyle::processSoEvent(const SoEvent * const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) {
        return inherited::processSoEvent(ev);
    }
    // Switch off viewing mode (Bug #0000911)
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
        const SoKeyboardEvent * const event = static_cast<const SoKeyboardEvent *>(ev);
        processed = processKeyboardEvent(event);
    }

    // Mouse Button / Spaceball Button handling
    if (type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent * const event = (const SoMouseButtonEvent *) ev;
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

        //SoDebugError::postInfo("processSoEvent", "button = %d", button);
        switch (button) {
        case SoMouseButtonEvent::BUTTON1:
            this->lockrecenter = true;
            this->button1down = press;
            if (press && (this->currentmode == NavigationStyle::SEEK_WAIT_MODE)) {
                newmode = NavigationStyle::SEEK_MODE;
                this->seekToPoint(pos); // implicitly calls interactiveCountInc()
                processed = true;
            }
            else if (press && (this->currentmode == NavigationStyle::PANNING ||
                               this->currentmode == NavigationStyle::ZOOMING)) {
                newmode = NavigationStyle::DRAGGING;
                saveCursorPosition(ev);
                this->centerTime = ev->getTime();
                processed = true;
            }
            else if (viewer->isEditing() && (this->currentmode == NavigationStyle::SPINNING)) {
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
            else {
                SbTime tmp = (ev->getTime() - this->centerTime);
                float dci = (float)QApplication::doubleClickInterval()/1000.0f;
                // is it just a middle click?
                if (tmp.getValue() < dci && !this->lockrecenter) {
                    if (!this->lookAtPoint(pos)) {
                        panToCenter(panningplane, posn);
                        this->interactiveCountDec();
                    }
                    processed = true;
                }
            }
            this->button3down = press;
            break;
        default:
            break;
        }
    }

    // Mouse Movement handling
    if (type.isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        this->lockrecenter = true;
        const SoLocation2Event * const event = (const SoLocation2Event *) ev;
        if (this->currentmode == NavigationStyle::ZOOMING) {
            this->zoomByCursor(posn, prevnormalized);
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
    }

    // Spaceball & Joystick handling
    if (type.isDerivedFrom(SoMotion3Event::getClassTypeId())) {
        const SoMotion3Event * const event = static_cast<const SoMotion3Event *>(ev);
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
        // The left mouse button has been released right now but
        // we want to avoid that the event is processed elsewhere
        if (this->lockButton1) {
            this->lockButton1 = false;
            processed = true;
        }
        break;
    case BUTTON1DOWN:
    case CTRLDOWN|BUTTON1DOWN:
        // make sure not to change the selection when stopping spinning
        if (curmode == NavigationStyle::SPINNING || this->lockButton1)
            newmode = NavigationStyle::IDLE;
        else
            newmode = NavigationStyle::SELECTION;
        break;
    case BUTTON1DOWN|BUTTON2DOWN:
        newmode = NavigationStyle::PANNING;
        break;
    case BUTTON3DOWN:
        newmode = NavigationStyle::PANNING;
        break;
    case SHIFTDOWN|BUTTON3DOWN:
        if (newmode != NavigationStyle::DRAGGING) {
            saveCursorPosition(ev);
        }
        newmode = NavigationStyle::DRAGGING;
        break;
    case CTRLDOWN|SHIFTDOWN|BUTTON2DOWN:
    case CTRLDOWN|BUTTON3DOWN:
        newmode = NavigationStyle::ZOOMING;
        break;

    default:
        break;
    }

    if (newmode != curmode) {
        this->setViewingMode(newmode);
    }

    // If for dragging the buttons 1 and 3 are pressed
    // but then button 3 is released we shouldn't switch
    // into selection mode.
    if (this->button1down && this->button3down)
        this->lockButton1 = true;

    // If not handled in this class, pass on upwards in the inheritance
    // hierarchy.
    if (!processed)
        processed = inherited::processSoEvent(ev);
    return processed;
}
