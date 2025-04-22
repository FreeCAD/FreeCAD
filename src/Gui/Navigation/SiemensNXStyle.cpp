// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#endif

#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/state.hpp>

#include "Camera.h"
#include "NavigationStyle.h"
#include "View3DInventorViewer.h"

// NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast,
//             cppcoreguidelines-avoid*,
//             readability-avoid-const-params-in-decls)
using namespace Gui;
namespace sc = boost::statechart;
using NS = SiemensNXStyle;

struct NS::Event: public sc::event<NS::Event>
{
    Event() : flags(new Flags){}
    using Button = SoMouseButtonEvent::Button;
    using Key = SoKeyboardEvent::Key;
    bool isMouseButtonEvent() const {
        return this->inventor_event->isOfType(SoMouseButtonEvent::getClassTypeId());
    }
    const SoMouseButtonEvent* asMouseButtonEvent() const {
        return static_cast<const SoMouseButtonEvent*>(this->inventor_event);
    }
    bool isPress(Button button) const {
        return SoMouseButtonEvent::isButtonPressEvent(this->inventor_event, button);
    }
    bool isRelease(Button button) const {
        return SoMouseButtonEvent::isButtonReleaseEvent(this->inventor_event, button);
    }
    bool isKeyPress(Key key) const {
        return SoKeyboardEvent::isKeyPressEvent(this->inventor_event, key);
    }
    bool isKeyRelease(Key key) const {
        return SoKeyboardEvent::isKeyReleaseEvent(this->inventor_event, key);
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
    const SoMotion3Event* asMotion3Event() const {
        return static_cast<const SoMotion3Event*>(this->inventor_event);
    }
    bool isDownButton(unsigned int state) const {
        return mbstate() == state;
    }
    bool isDownNoButton() const {
        return mbstate() == 0;
    }
    bool isDownButton1() const {
        return (mbstate() & BUTTON1DOWN) == BUTTON1DOWN;
    }
    bool isDownButton2() const {
        return (mbstate() & BUTTON2DOWN) == BUTTON2DOWN;
    }
    bool isDownButton3() const {
        return (mbstate() & BUTTON3DOWN) == BUTTON3DOWN;
    }
    bool isDownControl() const {
        return (kbstate() & CTRLDOWN) == CTRLDOWN;
    }
    bool isDownShift() const {
        return (kbstate() & SHIFTDOWN) == SHIFTDOWN;
    }
    bool isDownAlt() const {
        return (kbstate() & ALTDOWN) == ALTDOWN;
    }

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

    const SoEvent* inventor_event{nullptr};
    unsigned int modifiers{0};
    unsigned int mbstate() const {return modifiers & MASKBUTTONS;}
    unsigned int kbstate() const {return modifiers & MASKMODIFIERS;}

    struct Flags{
        bool processed = false;
        bool propagated = false;
    };
    std::shared_ptr<Flags> flags;
};

struct NS::NaviMachine: public sc::state_machine<NS::NaviMachine, NS::IdleState>
{
    using superclass = sc::state_machine<NS::NaviMachine, NS::IdleState>;
    explicit NaviMachine(NS& ns) : ns(ns) {}
    NS& ns;
};

struct NS::IdleState: public sc::state<NS::IdleState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;
    explicit IdleState(my_context ctx) : my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::IDLE);
    }
    sc::result react(const NS::Event& ev)
    {
        auto &ns = this->outermost_context().ns;
        switch (ns.getViewingMode()) {
        case NavigationStyle::SEEK_WAIT_MODE:
        {
            if (ev.isPress(SoMouseButtonEvent::BUTTON1)) {
                ns.seekToPoint(ev.inventor_event->getPosition());
                ns.setViewingMode(NavigationStyle::SEEK_MODE);
                ev.flags->processed = true;
                return transit<NS::AwaitingReleaseState>();
            }
            break;
        }
        case NavigationStyle::SPINNING:
        case NavigationStyle::SEEK_MODE:
        {
            if (!ev.flags->processed) {
                if (ev.isMouseButtonEvent()) {
                    ev.flags->processed = true;
                    return transit<NS::AwaitingReleaseState>();
                }
                else if (ev.isKeyboardEvent() || ev.isMotion3Event()) {
                    ns.setViewingMode(NavigationStyle::IDLE);
                }
            }

            break;
        }
        case NavigationStyle::BOXZOOM:
            return forward_event();
        }

        // right-click
        if (ev.isRelease(SoMouseButtonEvent::BUTTON2)
           && ev.mbstate() == 0
           && !ns.viewer->isEditing()
           && ns.isPopupMenuEnabled()){
            ns.openPopupMenu(ev.inventor_event->getPosition());
        }

        if (ev.isPress(SoMouseButtonEvent::BUTTON3)) {
            if (ev.isDownShift()) {
                ev.flags->processed = true;
                return transit<NS::PanState>();
            }

            if (ev.isDownButton(NS::Event::BUTTON3DOWN)) {
                ev.flags->processed = true;
                return transit<NS::AwaitingMoveState>();
            }
        }

        // Use processClickEvent()

        // Implement selection callback
        //if (ev.isLocation2Event() && ev.isDownButton1()) {
        //    ev.flags->processed = true;
        //    return transit<NS::SelectionState>();
        //}

        return forward_event();
    }
};

struct NS::AwaitingReleaseState : public sc::state<NS::AwaitingReleaseState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;
    explicit AwaitingReleaseState(my_context ctx) : my_base(ctx)
    {}

    sc::result react(const NS::Event& /*ev*/)
    {
        return forward_event();
    }
};

struct NS::InteractState: public sc::state<NS::InteractState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;
    explicit InteractState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::INTERACT);
    }

    sc::result react(const NS::Event& /*ev*/) {
        return forward_event();
    }
};

struct NS::AwaitingMoveState: public sc::state<NS::AwaitingMoveState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;

private:
    SbVec2s base_pos;
    SbTime since;

public:
    explicit AwaitingMoveState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::DRAGGING);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        this->since = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getTime();
    }
    sc::result react(const NS::Event& ev){
        //this state consumes all mouse events.
        ev.flags->processed = ev.isMouseButtonEvent() || ev.isLocation2Event();

        if (ev.isLocation2Event()) {
            return transit<NS::RotateState>();
        }

        // right-click
        if (ev.isPress(SoMouseButtonEvent::BUTTON2) && ev.isDownButton3()) {
            return transit<NS::PanState>();
        }

        if (ev.isKeyPress(SoKeyboardEvent::LEFT_SHIFT)) {
            ev.flags->processed = true;
            return transit<NS::PanState>();
        }

        // left-click
        if (ev.isPress(SoMouseButtonEvent::BUTTON1) && ev.isDownButton3()) {
            return transit<NS::ZoomState>();
        }

        if (ev.isKeyPress(SoKeyboardEvent::LEFT_CONTROL)) {
            ev.flags->processed = true;
            return transit<NS::ZoomState>();
        }

        // middle-click
        if (ev.isRelease(SoMouseButtonEvent::BUTTON3) && ev.isDownNoButton()) {
            auto &ns = this->outermost_context().ns;
            SbTime tmp = (ev.inventor_event->getTime() - this->since);
            double dci = QApplication::doubleClickInterval() / 1000.0;

            // is this a simple middle click?
            if (tmp.getValue() < dci) {
                ev.flags->processed = true;
                SbVec2s pos = ev.inventor_event->getPosition();
                ns.lookAtPoint(pos);
            }
            return transit<NS::IdleState>();
        }

        return forward_event();
    }
};

struct NS::RotateState: public sc::state<NS::RotateState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;
    explicit RotateState(my_context ctx) : my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        const auto inventorEvent = static_cast<const NS::Event*>(this->triggering_event())->inventor_event;
        ns.saveCursorPosition(inventorEvent);
        ns.setViewingMode(NavigationStyle::DRAGGING);
        this->base_pos = inventorEvent->getPosition();
    }

    sc::result react(const NS::Event& ev)
    {
        if (ev.isLocation2Event()) {
            auto &ns = this->outermost_context().ns;
            ns.addToLog(ev.inventor_event->getPosition(), ev.inventor_event->getTime());
            const SbVec2s pos = ev.inventor_event->getPosition();
            const SbVec2f posn = ns.normalizePixelPos(pos);
            ns.spin(posn);
            ns.moveCursorPosition();
            ev.flags->processed = true;
        }

        // right-click
        if (ev.isPress(SoMouseButtonEvent::BUTTON2) && ev.isDownButton3()) {
            ev.flags->processed = true;
            return transit<NS::PanState>();
        }

        if (ev.isKeyPress(SoKeyboardEvent::LEFT_SHIFT)) {
            ev.flags->processed = true;
            return transit<NS::PanState>();
        }

        // left-click
        if (ev.isPress(SoMouseButtonEvent::BUTTON1) && ev.isDownButton3()) {
            ev.flags->processed = true;
            return transit<NS::ZoomState>();
        }

        if (ev.isKeyPress(SoKeyboardEvent::LEFT_CONTROL)) {
            ev.flags->processed = true;
            return transit<NS::ZoomState>();
        }

        if (ev.isRelease(SoMouseButtonEvent::BUTTON3) && ev.isDownNoButton()) {
            ev.flags->processed = true;
            return transit<NS::IdleState>();
        }
        return forward_event();
    }

private:
    SbVec2s base_pos;
};

struct NS::PanState: public sc::state<NS::PanState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;
    explicit PanState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        const NS::Event* ev = static_cast<const NS::Event*>(this->triggering_event());
        ns.setViewingMode(NavigationStyle::PANNING);
        this->base_pos = ev->inventor_event->getPosition();
        this->ratio = ns.viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
        ns.centerTime = ev->inventor_event->getTime();
        ns.setupPanningPlane(ns.getCamera());
    }
    sc::result react(const NS::Event& ev)
    {
        if (ev.isLocation2Event()){
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

        if (ev.isRelease(SoMouseButtonEvent::BUTTON2) && ev.isDownButton3()) {
            ev.flags->processed = true;
            return transit<NS::RotateState>();
        }

        if (ev.isKeyRelease(SoKeyboardEvent::LEFT_SHIFT) && ev.isDownButton3()) {
            ev.flags->processed = true;
            return transit<NS::RotateState>();
        }

        if (ev.isRelease(SoMouseButtonEvent::BUTTON3)) {
            ev.flags->processed = true;
            return transit<NS::IdleState>();
        }

        return forward_event();
    }

private:
    SbVec2s base_pos;
    float ratio {1.0F};
};

struct NS::ZoomState: public sc::state<NS::ZoomState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;
    explicit ZoomState(my_context ctx) : my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        const NS::Event* ev = static_cast<const NS::Event*>(this->triggering_event());
        ns.setViewingMode(NavigationStyle::ZOOMING);
        this->base_pos = ev->inventor_event->getPosition();
    }

    sc::result react(const NS::Event& ev)
    {
        if (ev.isLocation2Event()){
            ev.flags->processed = true;
            SbVec2s pos = ev.inventor_event->getPosition();
            auto &ns = this->outermost_context().ns;
            ns.zoomByCursor(ns.normalizePixelPos(pos),
                            ns.normalizePixelPos(this->base_pos));
            this->base_pos = pos;
        }

        if (ev.isRelease(SoMouseButtonEvent::BUTTON1) && ev.isDownButton3()) {
            ev.flags->processed = true;
            return transit<NS::RotateState>();
        }

        if (ev.isKeyRelease(SoKeyboardEvent::LEFT_CONTROL) && ev.isDownButton3()) {
            ev.flags->processed = true;
            return transit<NS::RotateState>();
        }

        if (ev.isRelease(SoMouseButtonEvent::BUTTON3)) {
            ev.flags->processed = true;
            return transit<NS::IdleState>();
        }

        return forward_event();
    }

private:
    SbVec2s base_pos;
};

struct NS::SelectionState: public sc::state<NS::SelectionState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;
    explicit SelectionState(my_context ctx) : my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        const NS::Event* ev = static_cast<const NS::Event*>(this->triggering_event());

        ns.setViewingMode(NavigationStyle::BOXZOOM);
        ns.startSelection(NavigationStyle::Rubberband);
        fakeLeftButtonDown(ev->inventor_event->getPosition());
    }

    void fakeLeftButtonDown(const SbVec2s& pos)
    {
        SoMouseButtonEvent mbe;
        mbe.setButton(SoMouseButtonEvent::BUTTON1);
        mbe.setState(SoMouseButtonEvent::DOWN);
        mbe.setPosition(pos);

        auto &ns = this->outermost_context().ns;
        ns.processEvent(&mbe);
    }

    sc::result react(const NS::Event& /*ev*/)
    {
        // This isn't called while selection mode is active
        return transit<NS::IdleState>();
    }
};

// ----------------------------------------------------------------------------------

/* TRANSLATOR Gui::SiemensNXStyle */

TYPESYSTEM_SOURCE(Gui::SiemensNXStyle, Gui::UserNavigationStyle)

SiemensNXStyle::SiemensNXStyle()
    : naviMachine(new NS::NaviMachine(*this))
{
    naviMachine->initiate();
}

SiemensNXStyle::~SiemensNXStyle()
{
    naviMachine.reset();
}

const char* SiemensNXStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
    case NavigationStyle::SELECTION:
        return QT_TR_NOOP("Press left mouse button");
    case NavigationStyle::PANNING:
        return QT_TR_NOOP("Press middle+right click");
    case NavigationStyle::DRAGGING:
        return QT_TR_NOOP("Press middle mouse button");
    case NavigationStyle::ZOOMING:
        return QT_TR_NOOP("Scroll mouse wheel");
    default:
        return "No description";
    }
}

std::string SiemensNXStyle::userFriendlyName() const
{
    return {"Siemens NX"};
}

SbBool SiemensNXStyle::processKeyboardEvent(const SoKeyboardEvent * const event)
{
    // See https://forum.freecad.org/viewtopic.php?t=96459
    // Isometric view: Home key button
    // Trimetric view: End key button
    // Fit all: CTRL+F
    // Normal view: F8
    switch (event->getKey()) {
    case SoKeyboardEvent::F:
        if (event->wasCtrlDown()) {
            viewer->viewAll();
            return true;
        }
        break;
    case SoKeyboardEvent::HOME:
    {
        viewer->setCameraOrientation(Camera::rotation(Camera::Isometric));
        return true;
    }
    case SoKeyboardEvent::END:
    {
        viewer->setCameraOrientation(Camera::rotation(Camera::Trimetric));
        return true;
    }
    case SoKeyboardEvent::F8:
    {
        viewer->setCameraOrientation(Camera::rotation(Camera::Top));
        return true;
    }
    default:
        break;
    }

    return inherited::processKeyboardEvent(event);
}

SbBool SiemensNXStyle::processSoEvent(const SoEvent * const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) {
        return inherited::processSoEvent(ev);
    }

    // Switch off viewing mode (Bug #0000911)
    if (!this->isSeekMode() && !this->isAnimating() && this->isViewing()) {
        this->setViewing(false); // by default disable viewing mode to render the scene
    }

    // Mismatches in state of the modifier keys happens if the user
    // presses or releases them outside the viewer window.
    syncModifierKeys(ev);

    // give the nodes in the foreground root the chance to handle events (e.g color bar)
    if (!viewer->isEditing()) {
        if (handleEventInForeground(ev)) {
            return true;
        }
    }

    NS::Event smev;
    smev.inventor_event = ev;

    // Spaceball/joystick handling
    if (ev->isOfType(SoMotion3Event::getClassTypeId())){
        smev.flags->processed = true;
        this->processMotionEvent(static_cast<const SoMotion3Event*>(ev));
        return true;
    }

    // Keyboard handling
    if (ev->isOfType(SoKeyboardEvent::getClassTypeId())) {
        const auto event = static_cast<const SoKeyboardEvent *>(ev);
        smev.flags->processed = processKeyboardEvent(event);
    }

    if (smev.isMouseButtonEvent()) {
        const auto button = smev.asMouseButtonEvent()->getButton();
        const SbBool press = smev.asMouseButtonEvent()->getState() == SoButtonEvent::DOWN;
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
        default:
            break;
        }
    }

    smev.modifiers =
        (this->button1down ? NS::Event::BUTTON1DOWN : 0) |
        (this->button2down ? NS::Event::BUTTON2DOWN : 0) |
        (this->button3down ? NS::Event::BUTTON3DOWN : 0) |
        (this->ctrldown    ? NS::Event::CTRLDOWN : 0) |
        (this->shiftdown   ? NS::Event::SHIFTDOWN : 0) |
        (this->altdown     ? NS::Event::ALTDOWN : 0);

    if (!smev.flags->processed) {
        this->naviMachine->process_event(smev);
    }

    if (!smev.flags->propagated && !smev.flags->processed) {
        return inherited::processSoEvent(ev);
    }

    return smev.flags->processed;
}
// NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast,
//           cppcoreguidelines-avoid*,
//           readability-avoid-const-params-in-decls)
