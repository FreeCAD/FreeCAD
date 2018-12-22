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

/* TRANSLATOR Gui::CADNavigationStyle */

TYPESYSTEM_SOURCE(Gui::CADNavigationStyle, Gui::UserNavigationStyle);

CADNavigationStyle::CADNavigationStyle() : lockButton1(false)
{
}

CADNavigationStyle::~CADNavigationStyle()
{
}

const char* CADNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
    case NavigationStyle::SELECTION:
        return QT_TR_NOOP("Press left mouse button");
    case NavigationStyle::PANNING:
        return QT_TR_NOOP("Press middle mouse button");
    case NavigationStyle::DRAGGING:
        return QT_TR_NOOP("Press middle and left or right mouse button");
    case NavigationStyle::ZOOMING:
        return QT_TR_NOOP("Scroll middle mouse button or keep middle button pressed\n"
                          "while doing a left or right click and move the mouse up or down");
    default:
        return "No description";
    }
}

SbBool CADNavigationStyle::processSoEvent(const SoEvent * const ev)
{
#if 0
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) { return inherited::processSoEvent(ev); }
#else
    // Switch off viewing mode (Bug #0000911)
    if (!this->isSeekMode() && !this->isAnimating() && this->isViewing())
        this->setViewing(false); // by default disable viewing mode to render the scene
#endif

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

        // SoDebugError::postInfo("processSoEvent", "button = %d", button);
        switch (button) {
        case SoMouseButtonEvent::BUTTON1:
            this->lockrecenter = true;
            this->button1down = press;
#if 0 // disable to avoid interferences where this key combination is used, too
            if (press && ev->wasShiftDown() &&
                (this->currentmode != NavigationStyle::SELECTION)) {
                this->centerTime = ev->getTime();
                float ratio = vp.getViewportAspectRatio();
                SbViewVolume vv = viewer->getCamera()->getViewVolume(ratio);
                this->panningplane = vv.getPlane(viewer->getCamera()->focalDistance.getValue());
                this->lockrecenter = false;
            }
            else if (!press && ev->wasShiftDown() &&
                (this->currentmode != NavigationStyle::SELECTION)) {
                SbTime tmp = (ev->getTime() - this->centerTime);
                float dci = (float)QApplication::doubleClickInterval()/1000.0f;
                // is it just a left click?
                if (tmp.getValue() < dci && !this->lockrecenter) {
                    if (!this->moveToPoint(pos)) {
                        panToCenter(panningplane, posn);
                        this->interactiveCountDec();
                    }
                    processed = true;
                }
            }
            else
#endif
            if (press && (this->currentmode == NavigationStyle::SEEK_WAIT_MODE)) {
                newmode = NavigationStyle::SEEK_MODE;
                this->seekToPoint(pos); // implicitly calls interactiveCountInc()
                processed = true;
            }
            //else if (press && (this->currentmode == NavigationStyle::IDLE)) {
            //    this->setViewing(true);
            //    processed = true;
            //}
            else if (press && (this->currentmode == NavigationStyle::PANNING ||
                               this->currentmode == NavigationStyle::ZOOMING)) {
                newmode = NavigationStyle::DRAGGING;
                saveCursorPosition(ev);
                this->centerTime = ev->getTime();
                processed = true;
            }
            else if (!press && (this->currentmode == NavigationStyle::DRAGGING)) {
                SbTime tmp = (ev->getTime() - this->centerTime);
                float dci = (float)QApplication::doubleClickInterval()/1000.0f;
                if (tmp.getValue() < dci) {
                    newmode = NavigationStyle::ZOOMING;
                }
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
                SbTime tmp = (ev->getTime() - this->centerTime);
                float dci = (float)QApplication::doubleClickInterval()/1000.0f;
                if (tmp.getValue() < dci) {
                    newmode = NavigationStyle::ZOOMING;
                }
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
        case SoMouseButtonEvent::BUTTON4:
            doZoom(viewer->getSoRenderManager()->getCamera(), true, posn);
            processed = true;
            break;
        case SoMouseButtonEvent::BUTTON5:
            doZoom(viewer->getSoRenderManager()->getCamera(), false, posn);
            processed = true;
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

        //if (curmode == NavigationStyle::DRAGGING) {
        //    if (doSpin())
        //        newmode = NavigationStyle::SPINNING;
        //}
        break;
    case BUTTON1DOWN:
        // make sure not to change the selection when stopping spinning
        if (curmode == NavigationStyle::SPINNING || this->lockButton1)
            newmode = NavigationStyle::IDLE;
        else
            newmode = NavigationStyle::SELECTION;
        break;
    case BUTTON3DOWN:
        if (curmode == NavigationStyle::SPINNING) { break; }
        else if (newmode == NavigationStyle::ZOOMING) { break; }
        newmode = NavigationStyle::PANNING;

        if (curmode == NavigationStyle::DRAGGING) {
            if (doSpin()) {
                newmode = NavigationStyle::SPINNING;
                break;
            }
        }
        break;
    case CTRLDOWN|BUTTON2DOWN:
        newmode = NavigationStyle::PANNING;
        break;
    case SHIFTDOWN|BUTTON2DOWN:
        if (newmode != NavigationStyle::DRAGGING) {
            saveCursorPosition(ev);
        }
        newmode = NavigationStyle::DRAGGING;
        break;
    case CTRLDOWN|SHIFTDOWN|BUTTON2DOWN:
        newmode = NavigationStyle::ZOOMING;
        break;
    //case CTRLDOWN:
    //case CTRLDOWN|BUTTON1DOWN:
    //case CTRLDOWN|SHIFTDOWN:
    //case CTRLDOWN|SHIFTDOWN|BUTTON1DOWN:
    //    newmode = NavigationStyle::SELECTION;
    //    break;
    //case BUTTON1DOWN|BUTTON3DOWN:
    //case CTRLDOWN|BUTTON3DOWN:
    //    newmode = NavigationStyle::ZOOMING;
    //    break;

        // There are many cases we don't handle that just falls through to
        // the default case, like SHIFTDOWN, CTRLDOWN, CTRLDOWN|SHIFTDOWN,
        // SHIFTDOWN|BUTTON3DOWN, SHIFTDOWN|CTRLDOWN|BUTTON3DOWN, etc.
        // This is a feature, not a bug. :-)
        //
        // mortene.

    default:
        // The default will make a spin stop and otherwise not do
        // anything.
        //if ((curmode != NavigationStyle::SEEK_WAIT_MODE) &&
        //    (curmode != NavigationStyle::SEEK_MODE)) {
        //    newmode = NavigationStyle::IDLE;
        //}
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
    if (/*(curmode == NavigationStyle::SELECTION || viewer->isEditing()) && */!processed)
        processed = inherited::processSoEvent(ev);
    else
        return true;

    return processed;
}
