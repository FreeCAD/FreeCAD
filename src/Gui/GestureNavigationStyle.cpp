/***************************************************************************
 *   Copyright (c) 2019 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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
 *A few notes on this style. (by DeepSOIC)
 *
 * In this style, LMB serves dual purpose. It is selecting objects, as well as
 * spinning the view. The trick that enables it is to consume mouse events
 * before move threshold is detected, and refire the events if the mouse was
 * released but not moved.
 *
 * This navigation style does not exactly follow the structure of other
 * navigation styles, it does not fill many of the global variables defined in
 * NavigationStyle.
 *
 * It uses a statemachine based on boost::statechart to simplify differences in
 * event handling depending on mode.
 *
 * Dealing with touchscreen gestures with Qt5 on Windows is a pain in the arse.
 *
 * For pinch gesture, Qt5 starts generating mouse input as soon as the first
 * finger lands on the screen. As the second finger touches, gesture events
 * begin to arrive. But in the process, more synthetic mouse input keeps
 * coming, sometimes including RMB press. This mouse input is usually properly
 * terminated by LMB and RMB release events, but they don't always come;
 * proofing the logic against this inconsistency was quite a challenge.
 *
 * Tap-and-hold was yet another terrible beast. Again, as soon as the finger
 * touches the screen, LMB press comes in. Then, after the finger is released,
 * RMB press comes. This one is usually complemented by all release events.
 * However, with tap-hold-move-release sequence, RMB release event does not
 * arrive.
 *
 * So, to avoid entering Tilt mode, the style implements its own tap-and-hold
 * detection, and a special Pan state for the state machine - StickyPanState.
 *
 * This style wasn't tested with space mouse during development (I don't have one).
 *
 * See also GestureNavigationStyle-state-machine-diagram.docx for a crude
 * diagram of the state machine.
 *
 */

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Inventor/actions/SoRayPickAction.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/SoFullPath.h>
# include <Inventor/draggers/SoDragger.h>
# include <QApplication>
#endif

#include "GestureNavigationStyle.h"

#include <App/Application.h>
#include <Base/Console.h>
#include "View3DInventorViewer.h"
#include "Application.h"
#include "SoTouchEvents.h"

#include <QTapAndHoldGesture>

#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/custom_reaction.hpp>
#include <boost/mpl/list.hpp>

namespace sc = boost::statechart;
#define NS Gui::GestureNavigationStyle

namespace Gui {

class NS::Event : public sc::event<NS::Event>
{
public:
    Event():flags(new Flags){}
    virtual ~Event(){}

    void log() const {
        if (isPress(1))
            Base::Console().Log("button1 press ");
        if (isPress(2))
            Base::Console().Log("button2 press ");
        if (isPress(3))
            Base::Console().Log("button3 press ");
        if (isRelease(1))
            Base::Console().Log("button1 release ");
        if (isRelease(2))
            Base::Console().Log("button2 release ");
        if (isRelease(3))
            Base::Console().Log("button3 release ");
        if (isMouseButtonEvent())
            Base::Console().Log("%x", modifiers);
        if (isGestureEvent()){
            Base::Console().Log("Gesture ");
            switch(asGestureEvent()->state){
            case SoGestureEvent::SbGSStart:
                Base::Console().Log("start ");
            break;
            case SoGestureEvent::SbGSEnd:
                Base::Console().Log("end ");
            break;
            case SoGestureEvent::SbGSUpdate:
                Base::Console().Log("data ");
            break;
            default:
                Base::Console().Log("??? ");
            }

            Base::Console().Log(inventor_event->getTypeId().getName().getString());
        }
        if (isMouseButtonEvent() || isGestureEvent()){
            Base::Console().Log("(%i,%i)\n", inventor_event->getPosition()[0],inventor_event->getPosition()[1]);
        }
    }

    //cast shortcuts
    bool isMouseButtonEvent() const {
        return this->inventor_event->isOfType(SoMouseButtonEvent::getClassTypeId());
    }
    const SoMouseButtonEvent* asMouseButtonEvent() const {
        return static_cast<const SoMouseButtonEvent*>(this->inventor_event);
    }
    bool isPress(int button_index) const {
        if (! isMouseButtonEvent())
            return false;
        int sobtn = SoMouseButtonEvent::BUTTON1 + button_index - 1;
        return asMouseButtonEvent()->getButton() == sobtn && asMouseButtonEvent()->getState() == SoMouseButtonEvent::DOWN;
    }
    bool isRelease(int button_index) const {
        if (! isMouseButtonEvent())
            return false;
        int sobtn = SoMouseButtonEvent::BUTTON1 + button_index - 1;
        return asMouseButtonEvent()->getButton() == sobtn && asMouseButtonEvent()->getState() == SoMouseButtonEvent::UP;
    }
    bool isKeyboardEvent() const {
        return this->inventor_event->isOfType(SoKeyboardEvent::getClassTypeId());
    }
    const SoKeyboardEvent* asKeyboardEvent() const {
        return static_cast<const SoKeyboardEvent*>(this->inventor_event);
    }
    bool isLocation2Event() const {
        return this->inventor_event->isOfType(SoLocation2Event::getClassTypeId());
    }
    const SoLocation2Event* asLocation2Event() const {
        return static_cast<const SoLocation2Event*>(this->inventor_event);
    }
    bool isMotion3Event() const {
        return this->inventor_event->isOfType(SoMotion3Event::getClassTypeId());
    }
    bool isGestureEvent() const {
        return this->inventor_event->isOfType(SoGestureEvent::getClassTypeId());
    }
    const SoGestureEvent* asGestureEvent() const {
        return static_cast<const SoGestureEvent*>(this->inventor_event);
    }
    bool isGestureActive() const {
        if (!isGestureEvent())
            return false;
        if (asGestureEvent()->state == SoGestureEvent::SbGSStart
            || asGestureEvent()->state == SoGestureEvent::SbGSUpdate )
            return true;
        else
            return false;
    }
public:
    enum {
        // bits: 0-shift-ctrl-alt-0-lmb-mmb-rmb
        BUTTON1DOWN = 0x00000100,
        BUTTON2DOWN = 0x00000001,
        BUTTON3DOWN = 0x00000010,
        CTRLDOWN =    0x00100000,
        SHIFTDOWN =   0x01000000,
        ALTDOWN =     0x00010000,
        MASKBUTTONS = BUTTON1DOWN | BUTTON2DOWN | BUTTON3DOWN,
        MASKMODIFIERS = CTRLDOWN | SHIFTDOWN | ALTDOWN
    };

public:
    const SoEvent* inventor_event;
    unsigned int modifiers;
    unsigned int mbstate() const {return modifiers & MASKBUTTONS;}
    unsigned int kbdstate() const {return modifiers & MASKMODIFIERS;}

    struct Flags{
        bool processed = false; //the value to be returned by processSoEvent.
        bool propagated = false; //flag that the event had been passed to superclass
    };
    std::shared_ptr<Flags> flags;
    //storing these values as a separate unit allows to effectively write to
    //const object. Statechart passes all events as const, unfortunately, so
    //this is a workaround. I've considered casting away const instead, but
    //the internet seems to have mixed opinion if it's undefined behavior or
    //not. Also, it seems, statechart can copy the event internally. --DeepSOIC

};

//------------------------------state machine ---------------------------

class NS::NaviMachine : public sc::state_machine<NS::NaviMachine, NS::IdleState>
{
public:
    typedef sc::state_machine<NS::NaviMachine, NS::IdleState> superclass;

    NaviMachine(NS& ns) : ns(ns) {}
    NS& ns;

public:
    virtual void processEvent(NS::Event& ev) {
        ev.log();
        this->process_event(ev);
    }
};

class NS::IdleState : public sc::state<NS::IdleState, NS::NaviMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

    IdleState(my_context ctx):my_base(ctx)
    {
        this->outermost_context().ns.setViewingMode(NavigationStyle::IDLE);
        Base::Console().Log(" -> IdleState\n");
    }
    virtual ~IdleState(){}

    sc::result react(const NS::Event& ev){
        auto &ns = this->outermost_context().ns;

        auto posn = ns.normalizePixelPos(ev.inventor_event->getPosition());

        //special handling for some special states of viewer
        switch (ns.getViewingMode()) {
            case NavigationStyle::SEEK_WAIT_MODE:{
                if (ev.isPress(1)) {
                    ns.seekToPoint(ev.inventor_event->getPosition()); // implicitly calls interactiveCountInc()
                    ns.setViewingMode(NavigationStyle::SEEK_MODE);
                    ev.flags->processed = true;
                    return transit<NS::AwaitingReleaseState>();
                }
            } ; //not end of SEEK_WAIT_MODE. Fall through by design!!!
                /* FALLTHRU */
            case NavigationStyle::SPINNING:
            case NavigationStyle::SEEK_MODE: {
                //animation modes
                if (!ev.flags->processed) {
                    if (ev.isMouseButtonEvent()){
                        ev.flags->processed = true;
                        return transit<NS::AwaitingReleaseState>();
                    } else if (ev.isGestureEvent()
                                || ev.isKeyboardEvent()
                                || ev.isMotion3Event())
                        ns.setViewingMode(NavigationStyle::IDLE);
                }
            } break; //end of animation modes
            case BOXZOOM:
                return forward_event();
        }

        //testing for draggers
        if(ev.isPress(1) && ev.mbstate() == 0x100){
            if (ns.isDraggerUnderCursor(ev.inventor_event->getPosition()))
                return transit<NS::InteractState>();
        }

        //left and right clicks - special handling, postpone the events
        if (   (ev.isPress(1) && ev.mbstate() == 0x100)
            || (ev.isPress(2) && ev.mbstate() == 0x001)){
            ns.postponedEvents.post(ev);
            ev.flags->processed = true;
            return transit<NS::AwaitingMoveState>();
        }

        //MMB click
        if(ev.isPress(3) && ev.mbstate() == 0x010){
            ev.flags->processed = true;
            ns.onSetRotationCenter(ev.inventor_event->getPosition());
            return transit<NS::AwaitingReleaseState>();
        }

        //wheel events
        if(ev.isMouseButtonEvent() && ev.asMouseButtonEvent()->getButton() == SoMouseButtonEvent::BUTTON4){
            ns.doZoom(ns.viewer->getSoRenderManager()->getCamera(), true, posn);
            ev.flags->processed = true;
        }
        if(ev.isMouseButtonEvent() && ev.asMouseButtonEvent()->getButton() == SoMouseButtonEvent::BUTTON5){
            ns.doZoom(ns.viewer->getSoRenderManager()->getCamera(), false, posn);
            ev.flags->processed = true;
        }

        //touchscreen gestures
        if(ev.isGestureActive()){
            ev.flags->processed = true;
            return transit<NS::GestureState>();
        }

        //keyboard
        if(ev.isKeyboardEvent()){
            auto const &kbev = ev.asKeyboardEvent();
            ev.flags->processed = true;
            bool press = (kbev->getState() == SoKeyboardEvent::DOWN);
            switch (kbev->getKey()) {
                case SoKeyboardEvent::H:
                    if (press)
                        ns.onSetRotationCenter(kbev->getPosition());
                break;
                case SoKeyboardEvent::PAGE_UP:
                    if(press){
                        ns.doZoom(ns.viewer->getSoRenderManager()->getCamera(), true, posn);
                    }
                break;
                case SoKeyboardEvent::PAGE_DOWN:
                    if(press){
                        ns.doZoom(ns.viewer->getSoRenderManager()->getCamera(), false, posn);
                    }
                break;
                default:
                    ev.flags->processed = false;
            }
        }

        return forward_event();
    }
};

class NS::AwaitingMoveState : public sc::state<NS::AwaitingMoveState, NS::NaviMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;
    SbTime since; //the time of mouse-down event
    int hold_timeout; //in milliseconds

public:
    AwaitingMoveState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::IDLE);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        this->since = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getTime();

        ns.mouseMoveThreshold = App::GetApplication().GetParameterGroupByPath
                    ("User parameter:BaseApp/Preferences/View")->GetInt("GestureMoveThreshold", ns.mouseMoveThreshold);

        this->hold_timeout = int(double(QTapAndHoldGesture::timeout()) *0.9);
        this->hold_timeout = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/View")->GetInt("GestureTapHoldTimeout", this->hold_timeout);
        if (this->hold_timeout == 0)
            this->hold_timeout = 650; //a fail-safe
        QTapAndHoldGesture::setTimeout(int(double(this->hold_timeout)/0.9));
        // Why *0.9? We need tap-and-hold detection to be slightly faster than
        // Qt's one, to filter out spurious events. It'd be better to disable
        // Tap-and-hold altogether, but my attempts to use ungrabGesture and
        // unregisterRecognizer routines failed to affect anything.

    }
    virtual ~AwaitingMoveState(){
        //always clear postponed events when leaving this state.
        this->outermost_context().ns.postponedEvents.discardAll();
    }

    sc::result react(const NS::Event& ev){
        auto &ns = this->outermost_context().ns;

        ///refire(): forwards all postponed events + this event
        auto refire = [&]{
            ns.postponedEvents.forwardAll();
            ev.flags->processed = ns.processSoEvent_bypass(ev.inventor_event);
            ev.flags->propagated = true;
        };

        bool long_click = (ev.inventor_event->getTime() - this->since).getValue()*1000.0 >= this->hold_timeout;

        //this state consumes all mouse events.
        ev.flags->processed = ev.isMouseButtonEvent() || ev.isLocation2Event();

        //right-click
        if (ev.isRelease(2)
           && ev.mbstate() == 0
           && !ns.viewer->isEditing()
           && ns.isPopupMenuEnabled()){
            ns.openPopupMenu(ev.inventor_event->getPosition());
            return transit<NS::IdleState>();
        }

        //roll gestures
        //direction is determined at the moment the second button is pressed.
        if (ev.mbstate() == 0x101){
            if (ev.isPress(1))
                ns.rollDir = -1;
            if (ev.isPress(2))
                ns.rollDir = +1;
        }
        //The roll gesture is fired when one of the two buttons in then released.
        if (   (ev.isRelease(1) && ev.mbstate() == 0x001)
            || (ev.isRelease(2) && ev.mbstate() == 0x100) ){
            ns.onRollGesture(ns.rollDir);
            return transit<NS::AwaitingReleaseState>();
        }

        if(ev.isMouseButtonEvent() && ev.mbstate() == 0){
            //all buttons released
            if (long_click){
                //emulate RMB-click
                ns.openPopupMenu(ev.inventor_event->getPosition());
                return transit<NS::IdleState>();
            } else {
                //refire all events && return to idle state
                ns.setViewingMode(NavigationStyle::SELECTION);
                refire();
                return transit<NS::IdleState>();
            }
        }
        if (ev.isPress(3)){
            //mmb pressed, exit navigation
            refire();
            return transit<NS::IdleState>();
        }
        if (ev.isMouseButtonEvent() /* and still not processed*/){
            ns.postponedEvents.post(ev);
        }
        if(ev.isLocation2Event()){
            auto mv = ev.inventor_event->getPosition() - this->base_pos;
            if(SbVec2f(mv).length() > ns.mouseMoveThreshold)
            {
                //mouse moved while buttons are held. decide how to navigate...
                switch(ev.mbstate()){
                    case 0x100:{
                        if (!long_click) {
                            bool alt = ev.modifiers & NS::Event::ALTDOWN;
                            bool allowSpin = alt == ns.is2DViewing();
                            if(allowSpin)
                                return transit<NS::RotateState>();
                            else {
                                refire();
                                return transit<NS::IdleState>();
                            }
                        } else {
                            return transit<NS::StickyPanState>();
                        }
                    }break;
                    case 0x001:
                        return transit<NS::PanState>();
                    break;
                    case (0x101):
                        return transit<NS::TiltState>();
                    break;
                    default:
                        //MMB was held? refire all events.
                        refire();
                        return transit<NS::IdleState>();
                }
            }
        }
        if(ev.isGestureActive()){
            ev.flags->processed = true;
            return transit<NS::GestureState>();
        }
        return forward_event();
    }
};

class NS::RotateState : public sc::state<NS::RotateState, NS::NaviMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;

public:
    RotateState(my_context ctx):my_base(ctx)
    {
        this->outermost_context().ns.setViewingMode(NavigationStyle::DRAGGING);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        Base::Console().Log(" -> RotateState\n");
    }
    virtual ~RotateState(){}

    sc::result react(const NS::Event& ev){
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.mbstate() == 0x101){
                return transit<NS::TiltState>();
            }
            if (ev.mbstate() == 0){
                return transit<NS::IdleState>();
            }
        }
        if(ev.isLocation2Event()){
            ev.flags->processed = true;
            SbVec2s pos = ev.inventor_event->getPosition();
            auto &ns = this->outermost_context().ns;
            ns.spin_simplified(
                        ns.viewer->getSoRenderManager()->getCamera(),
                        ns.normalizePixelPos(pos), ns.normalizePixelPos(this->base_pos));
            this->base_pos = pos;
        }
        return forward_event();
    }
};

class NS::PanState : public sc::state<NS::PanState, NS::NaviMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;
    float ratio;

public:
    PanState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::PANNING);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        Base::Console().Log(" -> PanState\n");
        this->ratio = ns.viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
        ns.pan(ns.viewer->getSoRenderManager()->getCamera());//set up panningplane
    }
    virtual ~PanState(){}

    sc::result react(const NS::Event& ev){
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.mbstate() == 0x101){
                return transit<NS::TiltState>();
            }
            if (ev.mbstate() == 0){
                return transit<NS::IdleState>();
            }
        }
        if(ev.isLocation2Event()){
            ev.flags->processed = true;
            SbVec2s pos = ev.inventor_event->getPosition();
            auto &ns = this->outermost_context().ns;
            ns.panCamera(ns.viewer->getSoRenderManager()->getCamera(),
                         this->ratio,
                         ns.panningplane,
                         ns.normalizePixelPos(pos),
                         ns.normalizePixelPos(this->base_pos));
            this->base_pos = pos;
        }
        return forward_event();
    }
};

class NS::StickyPanState : public sc::state<NS::StickyPanState, NS::NaviMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;
    float ratio;

public:
    StickyPanState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::PANNING);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        Base::Console().Log(" -> StickyPanState\n");
        this->ratio = ns.viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
        ns.pan(ns.viewer->getSoRenderManager()->getCamera());//set up panningplane
    }
    virtual ~StickyPanState(){
        auto &ns = this->outermost_context().ns;
        ns.button2down = false; //a workaround for dealing with Qt not sending UP event after a tap-hold-drag sequence.
    }

    sc::result react(const NS::Event& ev){
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.isRelease(1)){
                return transit<NS::IdleState>();
            }
        }
        if(ev.isLocation2Event()){
            ev.flags->processed = true;
            SbVec2s pos = ev.inventor_event->getPosition();
            auto &ns = this->outermost_context().ns;
            ns.panCamera(ns.viewer->getSoRenderManager()->getCamera(),
                         this->ratio,
                         ns.panningplane,
                         ns.normalizePixelPos(pos),
                         ns.normalizePixelPos(this->base_pos));
            this->base_pos = pos;
        }
        return forward_event();
    }
};

class NS::TiltState : public sc::state<NS::TiltState, NS::NaviMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;

public:
    TiltState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::DRAGGING);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        Base::Console().Log(" -> TiltState\n");
        ns.pan(ns.viewer->getSoRenderManager()->getCamera());//set up panningplane
    }
    virtual ~TiltState(){}

    sc::result react(const NS::Event& ev){
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.mbstate() == 0x001){
                return transit<NS::PanState>();
            }
            if (ev.mbstate() == 0x100){
                return transit<NS::RotateState>();
            }
            if (ev.mbstate() == 0){
                return transit<NS::IdleState>();
            }
        }
        if(ev.isLocation2Event()){
            ev.flags->processed = true;
            auto &ns = this->outermost_context().ns;
            SbVec2s pos = ev.inventor_event->getPosition();
            float dx = (ns.normalizePixelPos(pos)-ns.normalizePixelPos(base_pos))[0];
            ns.doRotate(ns.viewer->getSoRenderManager()->getCamera(),
                        dx*(-2),
                        SbVec2f(0.5,0.5));
            this->base_pos = pos;
        }
        return forward_event();
    }
};


class NS::GestureState : public sc::state<NS::GestureState, NS::NaviMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;
    float ratio;
    bool enableTilt = false;

public:
    GestureState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::PANNING);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        Base::Console().Log(" -> GestureState\n");
        ns.pan(ns.viewer->getSoRenderManager()->getCamera());//set up panningplane
        this->ratio = ns.viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
        enableTilt = !(App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/View")->GetBool("DisableTouchTilt",true));
    }
    virtual ~GestureState(){
        auto &ns = this->outermost_context().ns;
        //a workaround for Qt not always sending release evends during touchecreen gestures on Windows
        ns.button1down = false;
        ns.button2down = false;
    }

    sc::result react(const NS::Event& ev){
        auto &ns = this->outermost_context().ns;
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.mbstate() == 0){
                //a fail-safe: if gesture end event doesn't arrive, a mouse click should be able to stop this mode.
                Base::Console().Warning("leaving gesture state by mouse-click (fail-safe)\n");
                return transit<NS::IdleState>();
            }
        }
        if(ev.isLocation2Event()){
            //consume all mouse events that Qt fires during the gesture (stupid Qt, so far it only causes trouble)
            ev.flags->processed = true;
        }
        if(ev.isGestureEvent()){
            ev.flags->processed = true;
            if(ev.asGestureEvent()->state == SoGestureEvent::SbGSEnd){
                return transit<NS::IdleState>();
            } else if (ev.asGestureEvent()->state == SoGestureEvent::SbGsCanceled){
                //should maybe undo the camera change caused by gesture events received so far...
                return transit<NS::IdleState>();
            //} else if (ev.asGestureEvent()->state == SoGestureEvent::SbGSStart){
            //    //ignore?
            } else if (ev.inventor_event->isOfType(SoGesturePanEvent::getClassTypeId())){
                auto const &pangesture = static_cast<const SoGesturePanEvent*>(ev.inventor_event);
                SbVec2f panDist = ns.normalizePixelPos(pangesture->deltaOffset);
                ns.panCamera(ns.viewer->getSoRenderManager()->getCamera(),
                             ratio,
                             ns.panningplane,
                             panDist,
                             SbVec2f(0,0));
            } else if (ev.inventor_event->isOfType(SoGesturePinchEvent::getClassTypeId())){
                const SoGesturePinchEvent* const pinch = static_cast<const SoGesturePinchEvent*>(ev.inventor_event);
                SbVec2f panDist = ns.normalizePixelPos(pinch->deltaCenter.getValue());
                ns.panCamera(ns.viewer->getSoRenderManager()->getCamera(),
                             ratio,
                             ns.panningplane,
                             panDist,
                             SbVec2f(0,0));
                ns.doZoom(ns.viewer->getSoRenderManager()->getCamera(),
                          -logf(float(pinch->deltaZoom)),
                          ns.normalizePixelPos(pinch->curCenter));
                if (pinch->deltaAngle != 0.0 && enableTilt){
                    ns.doRotate(ns.viewer->getSoRenderManager()->getCamera(),
                                float(pinch->deltaAngle),
                                ns.normalizePixelPos(pinch->curCenter));
                }
            } else {
                //unknown gesture
                ev.flags->processed = false;
            }
        }
        return forward_event();
    }
};

class NS::AwaitingReleaseState : public sc::state<NS::AwaitingReleaseState, NS::NaviMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

public:
    AwaitingReleaseState(my_context ctx):my_base(ctx)
    {
        Base::Console().Log(" -> AwaitingReleaseState\n");
    }
    virtual ~AwaitingReleaseState(){}

    sc::result react(const NS::Event& ev){
        auto &ns = this->outermost_context().ns;

        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.mbstate() == 0){
                return transit<NS::IdleState>();
            }
        }

        //roll gestures (same as in AwaitingMoveState, but how to merge them - I don't know  --DeepSOIC)
        //direction is determined at the moment the second button is pressed.
        if (ev.mbstate() == 0x101){
            if (ev.isPress(1))
                ns.rollDir = -1;
            if (ev.isPress(2))
                ns.rollDir = +1;
        }
        //The roll gesture is fired when one of the two buttons in then released.
        if (   (ev.isRelease(1) && ev.mbstate() == 0x001)
            || (ev.isRelease(2) && ev.mbstate() == 0x100) ){
            ns.onRollGesture(ns.rollDir);
        }

        if(ev.isLocation2Event()){
            ev.flags->processed = true;
        }
        if(ev.isGestureActive()){
            ev.flags->processed = true;
            //another gesture can start...
            return transit<NS::GestureState>();
        }
        return forward_event();
    }
};

class NS::InteractState : public sc::state<NS::InteractState, NS::NaviMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

public:
    InteractState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::INTERACT);
        Base::Console().Log(" -> InteractState\n");
    }
    virtual ~InteractState(){}

    sc::result react(const NS::Event& ev){
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = false; //feed all events to the dragger/whatever
            if (ev.mbstate() == 0){ //all buttons released?
                return transit<NS::IdleState>();
            }
        }
        return forward_event();
    }
};

//------------------------------/state machine ---------------------------


/* TRANSLATOR Gui::GestureNavigationStyle */

TYPESYSTEM_SOURCE(Gui::GestureNavigationStyle, Gui::UserNavigationStyle);


GestureNavigationStyle::GestureNavigationStyle()
    : naviMachine(new NS::NaviMachine(*this)),
      postponedEvents(*this)
{
    mouseMoveThreshold = QApplication::startDragDistance();
    naviMachine->initiate();

}

GestureNavigationStyle::~GestureNavigationStyle()
{

}

const char* GestureNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
    case NavigationStyle::SELECTION:
        return QT_TR_NOOP("Tap OR click left mouse button.");
    case NavigationStyle::PANNING:
        return QT_TR_NOOP("Drag screen with two fingers OR press right mouse button.");
    case NavigationStyle::DRAGGING:
        return QT_TR_NOOP("Drag screen with one finger OR press left mouse button. In Sketcher && other edit modes, hold Alt in addition.");
    case NavigationStyle::ZOOMING:
        return QT_TR_NOOP("Pinch (place two fingers on the screen && drag them apart from || towards each other) OR scroll middle mouse button OR PgUp/PgDown on keyboard.");
    default:
        return "No description";
    }
}


SbBool GestureNavigationStyle::processSoEvent(const SoEvent* const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) { return superclass::processSoEvent(ev); }
    // Switch off viewing mode (Bug #0000911)
    if (!this->isSeekMode()&& !this->isAnimating() && this->isViewing() )
        this->setViewing(false); // by default disable viewing mode to render the scene

    NS::Event smev;
    smev.inventor_event = ev;

    //mode-independent spaceball/joystick handling
    if (ev->isOfType(SoMotion3Event::getClassTypeId())){
        smev.flags->processed = true;
        this->processMotionEvent(static_cast<const SoMotion3Event*>(ev));
        return true;
    }

    // give the nodes in the foreground root the chance to handle events (e.g color bar)
    if (!viewer->isEditing()) {
        bool processed = handleEventInForeground(ev);
        if (processed) return true;
    }

    if (   (smev.isRelease(1) && this->button1down == false)
        || (smev.isRelease(2) && this->button2down == false)
        || (smev.isRelease(3) && this->button3down == false)) {
        //a button release event cane, but we didn't see the corresponding down
        //event. Discard it. This discarding is relied upon in some hacks to
        //overcome buggy synthetic mouse input coming from Qt when doing
        //touchecteen gestures.
        return true;
    }


    if (smev.isMouseButtonEvent()) {
        const int button = smev.asMouseButtonEvent()->getButton();
        const SbBool press //the button was pressed (if false -> released)
                = smev.asMouseButtonEvent()->getState() == SoButtonEvent::DOWN ? true : false;
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
    this->ctrldown = ev->wasCtrlDown();
    this->shiftdown = ev->wasShiftDown();
    this->altdown = ev->wasAltDown();

    smev.modifiers =
        (this->button1down ? NS::Event::BUTTON1DOWN : 0) |
        (this->button2down ? NS::Event::BUTTON2DOWN : 0) |
        (this->button3down ? NS::Event::BUTTON3DOWN : 0) |
        (this->ctrldown    ? NS::Event::CTRLDOWN : 0) |
        (this->shiftdown   ? NS::Event::SHIFTDOWN : 0) |
        (this->altdown     ? NS::Event::ALTDOWN : 0);

    if (! smev.flags->processed)
        this->naviMachine->processEvent(smev);
    if(! smev.flags->propagated && ! smev.flags->processed)
        return superclass::processSoEvent(ev);
    else
        return smev.flags->processed;
}

SbBool GestureNavigationStyle::processSoEvent_bypass(const SoEvent* const ev)
{
    return superclass::processSoEvent(ev);
}

bool GestureNavigationStyle::isDraggerUnderCursor(SbVec2s pos)
{
    SoRayPickAction rp(this->viewer->getSoRenderManager()->getViewportRegion());
    rp.setRadius(viewer->getPickRadius());
    rp.setPoint(pos);
    rp.apply(this->viewer->getSoRenderManager()->getSceneGraph());
    SoPickedPoint* pick = rp.getPickedPoint();
    if (pick){
        const SoFullPath* fullpath = static_cast<const SoFullPath*>(pick->getPath());
        for(int i = 0; i < fullpath->getLength(); ++i){
            if(fullpath->getNode(i)->isOfType(SoDragger::getClassTypeId()))
                return true;
        }
        return false;
    } else {
        return false;
    }
}

bool GestureNavigationStyle::is2DViewing() const
{
    // #FIXME: detect sketch editing, ! any editing
    return this->viewer->isEditing();
}

void GestureNavigationStyle::onRollGesture(int direction)
{
    std::string cmd;
    if (direction == +1){
        Base::Console().Log("Roll forward gesture\n");
        cmd = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/View")->GetASCII("GestureRollFwdCommand");
    } else if (direction == -1) {
        Base::Console().Log("Roll backward gesture\n");
        cmd = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/View")->GetASCII("GestureRollBackCommand");
    }
    if (cmd.size() == 0)
        return;
    std::stringstream code;
    code << "Gui.runCommand(\"" << cmd << "\")";
    try {
        Base::Interpreter().runString(code.str().c_str());
    } catch (Base::PyException exc) {
        exc.ReportException();
    } catch (...) {
        Base::Console().Error("GestureNavigationStyle::onRollGesture: unknown C++ exception when invoking command %s\n", cmd.c_str());
   }

}

void GestureNavigationStyle::onSetRotationCenter(SbVec2s cursor){
    SbBool ret = NavigationStyle::lookAtPoint(cursor);
    if(!ret){
        this->interactiveCountDec(); //this was in original gesture nav. Not sure what is it needed for --DeepSOIC
        Base::Console().Warning(
            "No object under cursor! Can't set new center of rotation.\n");
    }

}

void GestureNavigationStyle::EventQueue::post(const NS::Event& ev)
{
    ev.flags->processed = true;
    this->push(*ev.asMouseButtonEvent());
    Base::Console().Log("postponed: ");
    ev.log();
}

void GestureNavigationStyle::EventQueue::discardAll()
{
    while(! this->empty()){
        this->pop();
    }
}

void GestureNavigationStyle::EventQueue::forwardAll()
{
    while(! this->empty()){
        auto v = this->front();
        this->ns.processSoEvent_bypass(&v);
        this->pop();
    }
}

}//namespace Gui
