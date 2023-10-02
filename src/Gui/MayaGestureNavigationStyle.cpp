/***************************************************************************
 *   Copyright (c) 2015 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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
 ***************************************************************************
 *                                                                         *
 *   Minor modifications made by Pablo Gil (pablogil) in order to create   *
 *   a Maya or Unity 3D mouse navigation style:                            *
 *         ALT + left mouse button    = orbit                              *
 *         ALT + right mouse button   = zoom                               *
 *         ALT + middle mouse button  = pan                                *
 *                                                                         *
 *   Thanks Victor for your help!                                          *
 *                                                                         *
 ***************************************************************************/

/*
 *A few notes on this style. (by DeepSOIC)
 *
 * In this style, LMB serves dual purpose. It is selecting objects, as well as
 * spinning the view. The trick that enables it is as follows: The mousedown
 * event is consumed an saved, but otherwise remains unprocessed. If a drag is
 * detected while the button is down, the event is finally consumed (the saved
 * one is discarded), and spinning starts. If there is no drag detected before
 * the button is released, the saved mousedown is propagated to inherited,
 * followed by the mouseup. The same trick is used for RMB, so up to two
 * mousedown can be postponed.
 *
 * This navigation style does not exactly follow the structure of other
 * navigation styles, it does not fill many of the global variables defined in
 * NavigationStyle.
 *
 * This mode does not support locking cursor position on screen when
 * navigating, since with absolute pointing devices like pen and touch it makes
 * no sense (this style was specifically crafted for such devices).
 *
 * In this style, setViewing is not used (because I could not figure out how to
 * use it properly, and it seems to just work without it).
 *
 * This style wasn't tested with space during development (I don't have one).
 */

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QApplication>
#endif

#include <Base/Console.h>

#include "NavigationStyle.h"
#include "SoTouchEvents.h"
#include "View3DInventorViewer.h"


using namespace Gui;

// ----------------------------------------------------------------------------------

/* TRANSLATOR Gui::MayaGestureNavigationStyle */

TYPESYSTEM_SOURCE(Gui::MayaGestureNavigationStyle, Gui::UserNavigationStyle)

MayaGestureNavigationStyle::MayaGestureNavigationStyle()
{
    mouseMoveThreshold = QApplication::startDragDistance();
    mouseMoveThresholdBroken = false;
    mousedownConsumedCount = 0;
    thisClickIsComplex = false;
    inGesture = false;
}

MayaGestureNavigationStyle::~MayaGestureNavigationStyle() = default;

const char* MayaGestureNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
    case NavigationStyle::SELECTION:
        return QT_TR_NOOP("Tap OR click left mouse button.");
    case NavigationStyle::PANNING:
        return QT_TR_NOOP("Drag screen with two fingers OR press ALT + middle mouse button.");
    case NavigationStyle::DRAGGING:
        return QT_TR_NOOP("Drag screen with one finger OR press ALT + left mouse button. In Sketcher and other edit modes, hold Alt in addition.");
    case NavigationStyle::ZOOMING:
        return QT_TR_NOOP("Pinch (place two fingers on the screen and drag them apart from or towards each other) OR scroll middle mouse button OR press ALT + right mouse button OR PgUp/PgDown on keyboard.");
    default:
        return "No description";
    }
}

/*!
 * \brief MayaGestureNavigationStyle::testMoveThreshold tests if the mouse has moved far enough to constder it a drag.
 * \param currentPos current position of mouse cursor, in local pixel coordinates.
 * \return true if the mouse was moved far enough. False if it's within the boundary. Ignores MayaGestureNavigationStyle::mouseMoveThresholdBroken flag.
 */
bool MayaGestureNavigationStyle::testMoveThreshold(const SbVec2s currentPos) const {
    SbVec2s movedBy = currentPos - this->mousedownPos;
    return SbVec2f(movedBy).length() >= this->mouseMoveThreshold;
}

SbBool MayaGestureNavigationStyle::processSoEvent(const SoEvent * const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) {
        return inherited::processSoEvent(ev);
    }
    // Switch off viewing mode (Bug #0000911)
    if (!this->isSeekMode()&& !this->isAnimating() && this->isViewing() )
        this->setViewing(false); // by default disable viewing mode to render the scene
    //setViewing() is never used in this style, so the previous if is very unlikely to be hit.

    const SoType type(ev->getTypeId());
    //define some shortcuts...
    bool evIsButton = type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId());
    bool evIsKeyboard = type.isDerivedFrom(SoKeyboardEvent::getClassTypeId());
    bool evIsLoc2 = type.isDerivedFrom(SoLocation2Event::getClassTypeId());//mouse movement
    bool evIsLoc3 = type.isDerivedFrom(SoMotion3Event::getClassTypeId());//spaceball/joystick movement
    bool evIsGesture = type.isDerivedFrom(SoGestureEvent::getClassTypeId());//touchscreen gesture

    const SbVec2f prevnormalized = this->lastmouseposition;
    const SbVec2s pos(ev->getPosition());//not valid for gestures
    const SbVec2f posn = this->normalizePixelPos(pos);
    //pos: local coordinates of event, in pixels
    //posn: normalized local coordinates of event ((0,0) = lower left corner, (1,1) = upper right corner)
    float ratio = viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();

    if (evIsButton || evIsLoc2){
        this->lastmouseposition = posn;
    }

    const ViewerMode curmode = this->currentmode;
    //ViewerMode newmode = curmode;

    //make a unified mouse+modifiers state value (combo)
    enum {
        BUTTON1DOWN = 1 << 0,
        BUTTON2DOWN = 1 << 1,
        BUTTON3DOWN = 1 << 2,
        CTRLDOWN =    1 << 3,
        SHIFTDOWN =   1 << 4,
        ALTDOWN =     1 << 5,
        MASKBUTTONS = BUTTON1DOWN | BUTTON2DOWN | BUTTON3DOWN,
        MASKMODIFIERS = CTRLDOWN | SHIFTDOWN | ALTDOWN
    };
    unsigned int comboBefore = //before = state before this event
        (this->button1down ? BUTTON1DOWN : 0) |
        (this->button2down ? BUTTON2DOWN : 0) |
        (this->button3down ? BUTTON3DOWN : 0) |
        (this->ctrldown ? CTRLDOWN : 0) |
        (this->shiftdown ? SHIFTDOWN : 0) |
        (this->altdown ? ALTDOWN : 0);

    //test for complex clicks
    int cntMBBefore = (comboBefore & BUTTON1DOWN ? 1 : 0 ) //cntMBBefore = how many buttons were down when this event arrived?
                  +(comboBefore & BUTTON2DOWN ? 1 : 0 )
                  +(comboBefore & BUTTON3DOWN ? 1 : 0 );
    if (cntMBBefore>=2) this->thisClickIsComplex = true;
    if (cntMBBefore==0) {//a good chance to reset some click-related stuff
        this->thisClickIsComplex = false;
        this->mousedownConsumedCount = 0;//shouldn't be necessary, just a fail-safe.
    }

    // Mismatches in state of the modifier keys happens if the user
    // presses or releases them outside the viewer window.
    syncModifierKeys(ev);
    //before this block, mouse button states in NavigationStyle::buttonXdown reflected those before current event arrived.
    //track mouse button states
    if (evIsButton) {
        auto const event = (const SoMouseButtonEvent *) ev;
        const int button = event->getButton();
        const SbBool press //the button was pressed (if false -> released)
                = event->getState() == SoButtonEvent::DOWN ? true : false;
        switch (button) {
        case SoMouseButtonEvent::BUTTON1:
            this->button1down = press;
            break;
        case SoMouseButtonEvent::BUTTON2:
            this->button2down = press;
            break;
        case SoMouseButtonEvent::BUTTON3:
            this->button3down = press;
            break;
        //whatever else, we don't track
        }
    }
    //after this block, the new states of the buttons are already in.

    unsigned int comboAfter = //after = state after this event (current, essentially)
        (this->button1down ? BUTTON1DOWN : 0) |
        (this->button2down ? BUTTON2DOWN : 0) |
        (this->button3down ? BUTTON3DOWN : 0) |
        (this->ctrldown ? CTRLDOWN : 0) |
        (this->shiftdown ? SHIFTDOWN : 0) |
        (this->altdown ? ALTDOWN : 0);

    //test for complex clicks (again)
    int cntMBAfter = (comboAfter & BUTTON1DOWN ? 1 : 0 ) //cntMBAfter = how many buttons were down when this event arrived?
                  +(comboAfter & BUTTON2DOWN ? 1 : 0 )
                  +(comboAfter & BUTTON3DOWN ? 1 : 0 );
    if (cntMBAfter>=2) this->thisClickIsComplex = true;
    //if (cntMBAfter==0) this->thisClickIsComplex = false;//don't reset the flag now, we need to know that this mouseUp was an end of a click that was complex. The flag will reset by the before-check in the next event.

    //test for move detection
    if (evIsLoc2 || evIsButton){
        this->mouseMoveThresholdBroken |= this->testMoveThreshold(pos);
    }

    //track gestures
    if (evIsGesture) {
        auto gesture = static_cast<const SoGestureEvent*>(ev);
        switch(gesture->state) {
        case SoGestureEvent::SbGSStart:
            //assert(!inGesture);//start of another gesture before the first finished? Happens all the time for Pan gesture... No idea why!  --DeepSOIC
            inGesture = true;
        break;
        case SoGestureEvent::SbGSUpdate:
            assert(inGesture);//gesture update without start?
            inGesture = true;
        break;
        case SoGestureEvent::SbGSEnd:
            assert(inGesture);//gesture ended without starting?
            inGesture = false;
        break;
        case SoGestureEvent::SbGsCanceled:
            assert(inGesture);//gesture canceled without starting?
            inGesture=false;
        break;
        default:
            assert(0);//shouldn't happen
            inGesture = false;
        }
    }
    if (evIsButton) {
        if(inGesture){
            inGesture = false;//reset the flag when mouse clicks are received, to ensure enabling mouse navigation back.
            setViewingMode(NavigationStyle::SELECTION);//exit navigation asap, to proceed with regular processing of the click
        }
    }

    bool suppressLMBDrag = false;
    if(viewer->isEditing()){
        //in edit mode, disable lmb dragging (spinning). Holding Alt enables it.
        suppressLMBDrag = !(comboAfter & ALTDOWN);
    }

    //----------all this were preparations. Now comes the event handling! ----------

    SbBool processed = false;//a return value for the  BlahblahblahNavigationStyle::processSoEvent
    bool propagated = false;//an internal flag indicating that the event has been already passed to inherited, to suppress the automatic doing of this at the end.
    //goto finalize = return processed. Might be important to do something before done (none now).

    // give the nodes in the foreground root the chance to handle events (e.g color bar)
    if (!viewer->isEditing()) {
        processed = handleEventInForeground(ev);
    }
    if (processed)
        goto finalize;

    // Mode-independent keyboard handling
    if (evIsKeyboard) {
        auto const event = (const SoKeyboardEvent *) ev;
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;
        switch (event->getKey()) {
        case SoKeyboardEvent::H:
            processed = true;
            if(!press){
                SbBool ret = NavigationStyle::lookAtPoint(event->getPosition());
                if(!ret){
                    this->interactiveCountDec();
                    Base::Console().Log(
                        "No object under cursor! Can't set new center of rotation.\n");
                }
            }
            break;
        default:
            break;
        }
    }
    if (processed)
        goto finalize;

    //mode-independent spaceball/joystick handling
    if (evIsLoc3) {
        auto const event = static_cast<const SoMotion3Event *>(ev);
        if (event)
            this->processMotionEvent(event);
        processed = true;
    }
    if (processed)
        goto finalize;

    //all mode-dependent stuff is within this switch.
    switch(curmode){
    case NavigationStyle::IDLE:
    case NavigationStyle::SELECTION:
    case NavigationStyle::INTERACT: {
        //idle and interaction

        //keyboard
        if (evIsKeyboard) {
            auto const event = (const SoKeyboardEvent *) ev;
            const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

            switch(event->getKey()){
            case SoKeyboardEvent::S:
            case SoKeyboardEvent::HOME:
            case SoKeyboardEvent::LEFT_ARROW:
            case SoKeyboardEvent::UP_ARROW:
            case SoKeyboardEvent::RIGHT_ARROW:
            case SoKeyboardEvent::DOWN_ARROW:
                processed = inherited::processSoEvent(ev);
                propagated = true;
                break;
            case SoKeyboardEvent::PAGE_UP:
                if(press){
                    doZoom(viewer->getSoRenderManager()->getCamera(), getDelta(), posn);
                }
                processed = true;
                break;
            case SoKeyboardEvent::PAGE_DOWN:
                if(press){
                    doZoom(viewer->getSoRenderManager()->getCamera(), -getDelta(), posn);
                }
                processed = true;
                break;
            default:
                break;
            }//switch key
        }
        if (processed)
            goto finalize;


        // Mouse Button / Spaceball Button handling
        if (evIsButton) {
            auto const event = (const SoMouseButtonEvent *) ev;
            const int button = event->getButton();
            const SbBool press //the button was pressed (if false -> released)
                    = event->getState() == SoButtonEvent::DOWN ? true : false;
            switch(button){
            case SoMouseButtonEvent::BUTTON1:
            case SoMouseButtonEvent::BUTTON2:
                if(press){
                    if(this->thisClickIsComplex && this->mouseMoveThresholdBroken){
                        //this should prevent re-attempts to enter navigation when doing more clicks after a move.
                    } else {
                        //on LMB-down or RMB-down, we don't know yet if we should propagate it or process it. Save the event to be refired later, when it becomes clear.
                        //reset/start move detection machine
                        this->mousedownPos = pos;
                        this->mouseMoveThresholdBroken = false;
                        pan(viewer->getSoRenderManager()->getCamera());//set up panningplane
                        int &cnt = this->mousedownConsumedCount;
                        this->mousedownConsumedEvents[cnt] = *event;//hopefully, a shallow copy is enough. There are no pointers stored in events, apparently. Will lose a subclass, though.
                        cnt++;
                        assert(cnt<=2);
                        if(cnt>static_cast<int>(sizeof(mousedownConsumedEvents))){
                            cnt=sizeof(mousedownConsumedEvents);//we are in trouble
                        }
                        processed = true;//just consume this event, and wait for the move threshold to be broken to start dragging/panning
                    }
                } else {//release
                    if (button == SoMouseButtonEvent::BUTTON2 && !this->thisClickIsComplex) {
                        if (!viewer->isEditing() && this->isPopupMenuEnabled()) {
                            processed=true;
                            this->openPopupMenu(event->getPosition());
                        }
                    }
                    if(! processed) {
                        //re-synthesize all previously-consumed mouseDowns, if any. They might have been re-synthesized already when threshold was broken.
                        for( int i=0;   i < this->mousedownConsumedCount;   i++ ){
                            inherited::processSoEvent(& (this->mousedownConsumedEvents[i]));//simulate the previously-comsumed mousedown.
                        }
                        this->mousedownConsumedCount = 0;
                        processed = inherited::processSoEvent(ev);//explicitly, just for clarity that we are sending a full click sequence.
                        propagated = true;
                    }
                }
                break;
            case SoMouseButtonEvent::BUTTON3://press the wheel
                // starts PANNING mode
                if(press & this->altdown){
                    setViewingMode(NavigationStyle::PANNING);
                } else if(press){
                    // if not PANNING then look at point
                    SbBool ret = NavigationStyle::lookAtPoint(event->getPosition());
                    if(!ret){
                        this->interactiveCountDec();
                        Base::Console().Log(
                            "No object under cursor! Can't set new center of rotation.\n");
                    }
                }
                processed = true;
                break;
            }
        }

        //mouse moves - test for move threshold breaking
        if (evIsLoc2) {
            if (this->mouseMoveThresholdBroken && (this->button1down || this->button2down) && mousedownConsumedCount > 0) {
                //mousemovethreshold has JUST been broken

                //test if we should enter navigation
                if ((this->button1down && !suppressLMBDrag && this->altdown) || (this->button2down && this->altdown)) {
                    //yes, we are entering navigation.
                    //throw away consumed mousedowns.
                    this->mousedownConsumedCount = 0;

                    // start DRAGGING mode (orbit)
                    // if not pressing left mouse button then it assumes is right mouse button and starts ZOOMING mode
                    setViewingMode(this->button1down ? NavigationStyle::DRAGGING : NavigationStyle::ZOOMING);
                    processed = true;
                } else {
                    //no, we are not entering navigation.
                    //re-synthesize all previously-consumed mouseDowns, if any, and propagate this mousemove.
                    for( int i=0;   i < this->mousedownConsumedCount;   i++ ){
                        inherited::processSoEvent(& (this->mousedownConsumedEvents[i]));//simulate the previously-comsumed mousedown.
                    }
                    this->mousedownConsumedCount = 0;
                    processed = inherited::processSoEvent(ev);//explicitly, just for clarity that we are sending a full click sequence.
                    propagated = true;
                }
            }
            if (mousedownConsumedCount  > 0)
                processed = true;//if we are still deciding if it's a drag or not, consume mouseMoves.
        }

        //gesture start
        if (evIsGesture && /*!this->button1down &&*/ !this->button2down){//ignore gestures when mouse buttons are down. Button1down check was disabled because of wrong state after doubleclick on sketcher constraint to edit datum
            auto gesture = static_cast<const SoGestureEvent*>(ev);
            if (gesture->state == SoGestureEvent::SbGSStart
                    || gesture->state == SoGestureEvent::SbGSUpdate) {//even if we didn't get a start, assume the first update is a start (sort-of fail-safe).
                if (type.isDerivedFrom(SoGesturePanEvent::getClassTypeId())) {
                    pan(viewer->getSoRenderManager()->getCamera());//set up panning plane
                    setViewingMode(NavigationStyle::PANNING);
                    processed = true;
                } else if (type.isDerivedFrom(SoGesturePinchEvent::getClassTypeId())) {
                    pan(viewer->getSoRenderManager()->getCamera());//set up panning plane
                    setViewingMode(NavigationStyle::DRAGGING);
                    processed = true;
                } //all other gestures - ignore!
            }
        }

        //loc2 (mousemove) - ignore.

    } break;//end of idle and interaction
    case NavigationStyle::DRAGGING:
    case NavigationStyle::ZOOMING:
    case NavigationStyle::PANNING:{
        //actual navigation

        //no keyboard.

        // Mouse Button / Spaceball Button handling
        if (evIsButton) {
            auto const event = (const SoMouseButtonEvent *) ev;
            const int button = event->getButton();
            switch(button){
                case SoMouseButtonEvent::BUTTON1:
                case SoMouseButtonEvent::BUTTON2:
                case SoMouseButtonEvent::BUTTON3: // allows to release button3 into SELECTION mode
                    if(comboAfter & BUTTON1DOWN || comboAfter & BUTTON2DOWN) {
                        //don't leave navigation till all buttons have been released
                        setViewingMode((comboAfter & BUTTON1DOWN) ? NavigationStyle::DRAGGING : NavigationStyle::PANNING);
                        processed = true;
                    } else { //all buttons are released
                        //end of dragging/panning/whatever
                        setViewingMode(NavigationStyle::SELECTION);
                        processed = true;
                    } //end of else (some buttons down)
                break;
            } //switch(button)
        } //if(evIsButton)

        //the essence part 1!
        //mouse movement into camera motion. Suppress if in gesture. Ignore until threshold is surpassed.
        if (evIsLoc2 && ! this->inGesture && this->mouseMoveThresholdBroken) {
            if (curmode == NavigationStyle::ZOOMING) {//doesn't happen
                this->zoomByCursor(posn, prevnormalized);
                processed = true;
            } else if (curmode == NavigationStyle::PANNING) {
                panCamera(viewer->getSoRenderManager()->getCamera(), ratio, this->panningplane, posn, prevnormalized);
                processed = true;
            } else if (curmode == NavigationStyle::DRAGGING) {
                if (comboAfter & BUTTON1DOWN && comboAfter & BUTTON2DOWN) {
                    //two mouse buttons down - tilting!
                    NavigationStyle::doRotate(viewer->getSoRenderManager()->getCamera(),
                                              (posn - prevnormalized)[0]*(-2),
                                              SbVec2f(0.5,0.5));
                    processed = true;
                } else {//one mouse button - normal spinning
                    //this will also handle the single-finger drag (there's no gesture used, pseudomouse is enough)
                    //this->addToLog(event->getPosition(), event->getTime());
                    this->spin_simplified(viewer->getSoRenderManager()->getCamera(),
                                          posn, prevnormalized);
                    processed = true;
                }
            }
        }

        //the essence part 2!
        //gesture into camera motion
        if (evIsGesture){
            auto gesture = static_cast<const SoGestureEvent*>(ev);
            assert(gesture);
            if (gesture->state == SoGestureEvent::SbGSEnd) {
                setViewingMode(NavigationStyle::SELECTION);
                processed=true;
            } else if (gesture->state == SoGestureEvent::SbGSUpdate){
                if(type.isDerivedFrom(SoGesturePinchEvent::getClassTypeId())){
                    auto const event = static_cast<const SoGesturePinchEvent*>(ev);
                    if (this->zoomAtCursor){
                        //this is just dealing with the pan part of pinch gesture. Taking care of zooming to pos is done in doZoom.
                        SbVec2f panDist = this->normalizePixelPos(event->deltaCenter.getValue());
                        NavigationStyle::panCamera(viewer->getSoRenderManager()->getCamera(), ratio, this->panningplane, panDist, SbVec2f(0,0));
                    }
                    NavigationStyle::doZoom(viewer->getSoRenderManager()->getCamera(),-logf(event->deltaZoom),this->normalizePixelPos(event->curCenter));
                    if (event->deltaAngle != 0)
                        NavigationStyle::doRotate(viewer->getSoRenderManager()->getCamera(),event->deltaAngle,this->normalizePixelPos(event->curCenter));
                    processed = true;
                }
                if(type.isDerivedFrom(SoGesturePanEvent::getClassTypeId())){
                    auto const event = static_cast<const SoGesturePanEvent*>(ev);
                        //this is just dealing with the pan part of pinch gesture. Taking care of zooming to pos is done in doZoom.
                    SbVec2f panDist = this->normalizePixelPos(event->deltaOffset);
                    NavigationStyle::panCamera(viewer->getSoRenderManager()->getCamera(), ratio, this->panningplane, panDist, SbVec2f(0,0));
                    processed = true;
                }
            } else {
                //shouldn't happen. Gestures are not expected to start in the middle of navigation.
                //we'll consume it, without reacting.
                processed=true;
            }
        }

    } break;//end of actual navigation
    case NavigationStyle::SEEK_WAIT_MODE:{
        if (evIsButton) {
            auto const event = (const SoMouseButtonEvent *) ev;
            const int button = event->getButton();
            const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;
            if (button == SoMouseButtonEvent::BUTTON1 && press) {
                this->seekToPoint(pos); // implicitly calls interactiveCountInc()
                this->setViewingMode(NavigationStyle::SEEK_MODE);
                processed = true;
            }
        }
    } ; //not end of SEEK_WAIT_MODE. Fall through by design!!!
        /* FALLTHRU */
    case NavigationStyle::SPINNING:
    case NavigationStyle::SEEK_MODE: {
        //animation modes
        if (!processed) {
            if (evIsButton || evIsGesture || evIsKeyboard || evIsLoc3)
                setViewingMode(NavigationStyle::SELECTION);
        }
    } break; //end of animation modes
    case NavigationStyle::BOXZOOM:
    default:
        //all the rest - will be pass on to inherited, later.
        break;
    }

    if (! processed && ! propagated) {
        processed = inherited::processSoEvent(ev);
        propagated = true;
    }

    //-----------------------end of event handling---------------------
finalize:
    return processed;
}
