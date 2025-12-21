// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2025 Jonathan                                           *
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


#include <Inventor/nodes/SoCamera.h>
#include <QApplication>


#include "Navigation/NavigationStyle.h"
#include "View3DInventorViewer.h"


using namespace Gui;

// ----------------------------------------------------------------------------------

/* TRANSLATOR Gui::FusionNavigationStyle */

TYPESYSTEM_SOURCE(Gui::FusionNavigationStyle, Gui::UserNavigationStyle)

FusionNavigationStyle::FusionNavigationStyle()
    : lockButton1(false)
{}

FusionNavigationStyle::~FusionNavigationStyle() = default;

const char* FusionNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
        case NavigationStyle::SELECTION:
            return QT_TR_NOOP("Press left mouse button");
        case NavigationStyle::PANNING:
            return QT_TR_NOOP("Press middle mouse button");
        case NavigationStyle::DRAGGING:
            return QT_TR_NOOP("Press Shift and middle mouse button");
        case NavigationStyle::ZOOMING:
            return QT_TR_NOOP("Scroll middle mouse button");
        default:
            return "No description";
    }
}

SbBool FusionNavigationStyle::processSoEvent(const SoEvent* const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) {
        return inherited::processSoEvent(ev);
    }
    // Switch off viewing mode (Bug #0000911)
    if (!this->isSeekMode() && !this->isAnimating() && this->isViewing()) {
        this->setViewing(false);  // by default disable viewing mode to render the scene
    }

    const SoType type(ev->getTypeId());

    const SbViewportRegion& vp = viewer->getSoRenderManager()->getViewportRegion();
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
        if (processed) {
            return true;
        }
    }

    // Keyboard handling
    if (type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const auto event = static_cast<const SoKeyboardEvent*>(ev);
        processed = processKeyboardEvent(event);
    }

    // Mouse Button / Spaceball Button handling
    if (type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const auto event = (const SoMouseButtonEvent*)ev;
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

        // SoDebugError::postInfo("processSoEvent", "button = %d", button);
        switch (button) {
            case SoMouseButtonEvent::BUTTON1:
                this->lockrecenter = true;
                this->button1down = press;
                if (press && (this->currentmode == NavigationStyle::SEEK_WAIT_MODE)) {
                    newmode = NavigationStyle::SEEK_MODE;
                    this->seekToPoint(pos);  // implicitly calls interactiveCountInc()
                    processed = true;
                }
                else if (press
                         && (this->currentmode == NavigationStyle::PANNING
                             || this->currentmode == NavigationStyle::ZOOMING)) {
                    newmode = NavigationStyle::DRAGGING;
                    saveCursorPosition(ev);
                    this->centerTime = ev->getTime();
                    processed = true;
                }
                else if (!press && (this->currentmode == NavigationStyle::DRAGGING)) {
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

                // Don't show the context menu after dragging, panning or zooming
                if (!press && (hasDragged || hasPanned || hasZoomed)) {
                    processed = true;
                }
                else if (!press && !viewer->isEditing()) {
                    if (this->currentmode != NavigationStyle::ZOOMING
                        && this->currentmode != NavigationStyle::PANNING
                        && this->currentmode != NavigationStyle::DRAGGING) {
                        if (this->isPopupMenuEnabled()) {
                            this->openPopupMenu(event->getPosition());
                        }
                    }
                }
                // Alternative way of rotating & zooming
                if (press
                    && (this->currentmode == NavigationStyle::PANNING
                        || this->currentmode == NavigationStyle::ZOOMING)) {
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
                    setupPanningPlane(getCamera());
                    this->lockrecenter = false;
                }
                else {
                    SbTime tmp = (ev->getTime() - this->centerTime);
                    float dci = (float)QApplication::doubleClickInterval() / 1000.0f;
                    // is it just a middle click?
                    if (tmp.getValue() < dci && !this->lockrecenter) {
                        lookAtPoint(pos);
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
        const auto event = (const SoLocation2Event*)ev;
        if (this->currentmode == NavigationStyle::ZOOMING) {
            this->zoomByCursor(posn, prevnormalized);
            processed = true;
        }
        else if (this->currentmode == NavigationStyle::PANNING) {
            float ratio = vp.getViewportAspectRatio();
            panCamera(
                viewer->getSoRenderManager()->getCamera(),
                ratio,
                this->panningplane,
                posn,
                prevnormalized
            );
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
        const auto event = static_cast<const SoMotion3Event*>(ev);
        if (event) {
            this->processMotionEvent(event);
        }
        processed = true;
    }

    enum
    {
        BUTTON1DOWN = 1 << 0,
        BUTTON3DOWN = 1 << 1,
        CTRLDOWN = 1 << 2,
        SHIFTDOWN = 1 << 3,
        BUTTON2DOWN = 1 << 4
    };
    unsigned int combo = (this->button1down ? BUTTON1DOWN : 0)
        | (this->button2down ? BUTTON2DOWN : 0) | (this->button3down ? BUTTON3DOWN : 0)
        | (this->ctrldown ? CTRLDOWN : 0) | (this->shiftdown ? SHIFTDOWN : 0);

    switch (combo) {
        case 0:
            if (curmode == NavigationStyle::SPINNING) {
                break;
            }
            newmode = NavigationStyle::IDLE;
            // The left mouse button has been released right now
            if (this->lockButton1) {
                this->lockButton1 = false;
                if (curmode != NavigationStyle::SELECTION) {
                    processed = true;
                }
            }
            break;
        case BUTTON1DOWN:
        case CTRLDOWN | BUTTON1DOWN:
            // make sure not to change the selection when stopping spinning
            if (curmode == NavigationStyle::SPINNING
                || (this->lockButton1 && curmode != NavigationStyle::SELECTION)) {
                newmode = NavigationStyle::IDLE;
            }
            else {
                newmode = NavigationStyle::SELECTION;
            }
            break;
        case BUTTON3DOWN:
            if (curmode == NavigationStyle::DRAGGING) {
                 // Stick in DRAGGING mode even if shift is released
                 newmode = NavigationStyle::DRAGGING;
            } else {
                 newmode = NavigationStyle::PANNING;
            }
            break;
        case SHIFTDOWN | BUTTON3DOWN:
            if (newmode != NavigationStyle::DRAGGING) {
                if (curmode == NavigationStyle::PANNING) {
                     // Do NOT switch to dragging if already panning (Shift press during pan)
                     newmode = NavigationStyle::PANNING;
                } else {
                    saveCursorPosition(ev);
                    newmode = NavigationStyle::DRAGGING;
                }
            } else {
                newmode = NavigationStyle::DRAGGING;
            }
            break;
        case CTRLDOWN | SHIFTDOWN | BUTTON2DOWN:
        case CTRLDOWN | BUTTON3DOWN:
            newmode = NavigationStyle::ZOOMING;
            break;

        default:
            // Reset mode to IDLE when button 3 is released
            // This stops the DRAGGING when button 3 is released but SHIFT is still pressed
            // This stops the ZOOMING when button 3 is released but CTRL is still pressed
            if ((curmode == NavigationStyle::DRAGGING || curmode == NavigationStyle::ZOOMING)
                && !this->button3down) {
                newmode = NavigationStyle::IDLE;
            }
            break;
    }

    // If the selection button is pressed together with another button
    // and the other button is released, don't switch to selection mode.
    // Process when selection button is pressed together with other buttons that could trigger
    // different actions.
    if (this->button1down && (this->button2down || this->button3down)) {
        this->lockButton1 = true;
        processed = true;
    }

    // Prevent interrupting rubber-band selection in sketcher
    if (viewer->isEditing() && curmode == NavigationStyle::SELECTION
        && newmode != NavigationStyle::IDLE) {
        if (!button1down || !button2down) {  // Allow canceling rubber-band in sketcher if both
                                             // button 1 and button 2 are pressed
            newmode = NavigationStyle::SELECTION;
        }
        processed = false;
    }

    // Reset flags when newmode is IDLE and the buttons are released
    if (newmode == IDLE && !button1down && !button2down && !button3down) {
        hasPanned = false;
        hasDragged = false;
        hasZoomed = false;
    }

    if (newmode != curmode) {
        this->setViewingMode(newmode);
    }

    // If not handled in this class, pass on upwards in the inheritance
    // hierarchy.
    if (!processed) {
        processed = inherited::processSoEvent(ev);
    }
    return processed;
}

