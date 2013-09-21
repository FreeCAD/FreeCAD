/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

/* TRANSLATOR Gui::TouchpadNavigationStyle */

TYPESYSTEM_SOURCE(Gui::TouchpadNavigationStyle, Gui::UserNavigationStyle);

TouchpadNavigationStyle::TouchpadNavigationStyle()
{
}

TouchpadNavigationStyle::~TouchpadNavigationStyle()
{
}

const char* TouchpadNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
    case NavigationStyle::SELECTION:
        return QT_TR_NOOP("Press left mouse button");
    case NavigationStyle::PANNING:
        return QT_TR_NOOP("Press SHIFT button");
    case NavigationStyle::DRAGGING:
        return QT_TR_NOOP("Press ALT button");
    case NavigationStyle::ZOOMING:
        return QT_TR_NOOP("Press CTRL and SHIFT buttons");
    default:
        return "No description";
    }
}

SbBool TouchpadNavigationStyle::processSoEvent(const SoEvent * const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) { return inherited::processSoEvent(ev); }
    // Switch off viewing mode (Bug #0000911)
    if (!this->isSeekMode() && !this->isAnimating() && this->isViewing())
        this->setViewing(false); // by default disable viewing mode to render the scene

    const SoType type(ev->getTypeId());

    const SbViewportRegion & vp = viewer->getViewportRegion();
    const SbVec2s size(vp.getViewportSizePixels());
    const SbVec2f prevnormalized = this->lastmouseposition;
    const SbVec2s pos(ev->getPosition());
    const SbVec2f posn((float) pos[0] / (float) SoQtMax((int)(size[0] - 1), 1),
                       (float) pos[1] / (float) SoQtMax((int)(size[1] - 1), 1));

    this->lastmouseposition = posn;

    // Set to TRUE if any event processing happened. Note that it is not
    // necessary to restrict ourselves to only do one "action" for an
    // event, we only need this flag to see if any processing happened
    // at all.
    SbBool processed = FALSE;

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
            return TRUE;
    }

    // Keyboard handling
    if (type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const SoKeyboardEvent * const event = (const SoKeyboardEvent *) ev;
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;
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
            processed = TRUE;
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
        case SoKeyboardEvent::PAGE_UP:
            doZoom(viewer->getCamera(), TRUE, posn);
            processed = TRUE;
            break;
        case SoKeyboardEvent::PAGE_DOWN:
            doZoom(viewer->getCamera(), FALSE, posn);
            processed = TRUE;
            break;
        default:
            break;
        }
    }

    // Mouse Button / Spaceball Button handling
    if (type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent * const event = (const SoMouseButtonEvent *) ev;
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;

        // SoDebugError::postInfo("processSoEvent", "button = %d", button);
        switch (button) {
        case SoMouseButtonEvent::BUTTON1:
            this->lockrecenter = TRUE;
            this->button1down = press;
            if (press && (this->currentmode == NavigationStyle::SEEK_WAIT_MODE)) {
                newmode = NavigationStyle::SEEK_MODE;
                this->seekToPoint(pos); // implicitly calls interactiveCountInc()
                processed = TRUE;
            }
            else if (press && (this->currentmode == NavigationStyle::PANNING ||
                               this->currentmode == NavigationStyle::ZOOMING)) {
                newmode = NavigationStyle::DRAGGING;
                saveCursorPosition(ev);
                this->centerTime = ev->getTime();
                processed = TRUE;
            }
            else if (!press && (this->currentmode == NavigationStyle::DRAGGING)) {
                SbTime tmp = (ev->getTime() - this->centerTime);
                float dci = (float)QApplication::doubleClickInterval()/1000.0f;
                if (tmp.getValue() < dci) {
                    newmode = NavigationStyle::ZOOMING;
                }
                processed = TRUE;
            }
            else if (!press && (this->currentmode == NavigationStyle::DRAGGING)) {
                this->setViewing(false);
                processed = TRUE;
            }
            else if (viewer->isEditing() && (this->currentmode == NavigationStyle::SPINNING)) {
                processed = TRUE;
            }
            break;
        case SoMouseButtonEvent::BUTTON2:
            // If we are in edit mode then simply ignore the RMB events
            // to pass the event to the base class.
            this->lockrecenter = TRUE;
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
                processed = TRUE;
            }
            else if (!press && (this->currentmode == NavigationStyle::DRAGGING)) {
                SbTime tmp = (ev->getTime() - this->centerTime);
                float dci = (float)QApplication::doubleClickInterval()/1000.0f;
                if (tmp.getValue() < dci) {
                    newmode = NavigationStyle::ZOOMING;
                }
                processed = TRUE;
            }
            this->button2down = press;
            break;
        case SoMouseButtonEvent::BUTTON4:
            doZoom(viewer->getCamera(), TRUE, posn);
            processed = TRUE;
            break;
        case SoMouseButtonEvent::BUTTON5:
            doZoom(viewer->getCamera(), FALSE, posn);
            processed = TRUE;
            break;
        default:
            break;
        }
    }

    // Mouse Movement handling
    if (type.isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        this->lockrecenter = TRUE;
        const SoLocation2Event * const event = (const SoLocation2Event *) ev;
        if (this->currentmode == NavigationStyle::ZOOMING) {
            this->zoomByCursor(posn, prevnormalized);
            processed = TRUE;
        }
        else if (this->currentmode == NavigationStyle::PANNING) {
            float ratio = vp.getViewportAspectRatio();
            panCamera(viewer->getCamera(), ratio, this->panningplane, posn, prevnormalized);
            processed = TRUE;
        }
        else if (this->currentmode == NavigationStyle::DRAGGING) {
            this->addToLog(event->getPosition(), event->getTime());
            this->spin(posn);
            moveCursorPosition();
            processed = TRUE;
        }
    }

    // Spaceball & Joystick handling
    if (type.isDerivedFrom(SoMotion3Event::getClassTypeId())) {
        const SoMotion3Event * const event = static_cast<const SoMotion3Event * const>(ev);
        if (event)
            this->processMotionEvent(event);
        processed = TRUE;
    }

    enum {
        BUTTON1DOWN = 1 << 0,
        BUTTON2DOWN = 1 << 1,
        CTRLDOWN =    1 << 2,
        SHIFTDOWN =   1 << 3,
        ALTDOWN =     1 << 4
    };
    unsigned int combo =
        (this->button1down ? BUTTON1DOWN : 0) |
        (this->button2down ? BUTTON2DOWN : 0) |
        (this->ctrldown ? CTRLDOWN : 0) |
        (this->shiftdown ? SHIFTDOWN : 0) |
        (this->altdown ? ALTDOWN : 0);

    switch (combo) {
    case 0:
        if (curmode == NavigationStyle::SPINNING) { break; }
        newmode = NavigationStyle::IDLE;
        break;
    case BUTTON1DOWN:
        // make sure not to change the selection when stopping spinning
        if (curmode == NavigationStyle::SPINNING)
            newmode = NavigationStyle::IDLE;
        else
            newmode = NavigationStyle::SELECTION;
        break;
    case CTRLDOWN:
        newmode = NavigationStyle::IDLE;
        break;
    case SHIFTDOWN:
        newmode = NavigationStyle::PANNING;
        break;
    case ALTDOWN:
        if (newmode != NavigationStyle::DRAGGING) {
            saveCursorPosition(ev);
        }
        newmode = NavigationStyle::DRAGGING;
        break;
    case CTRLDOWN|SHIFTDOWN:
    case CTRLDOWN|SHIFTDOWN|BUTTON1DOWN:
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
    else
        return TRUE;

    return processed;
}
