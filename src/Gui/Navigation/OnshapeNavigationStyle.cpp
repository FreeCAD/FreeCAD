// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
#endif

#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/state.hpp>

#include "Camera.h"
#include "OnShapeNavigationStyle.h"
#include "View3DInventorViewer.h"

// NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast,
//             cppcoreguidelines-avoid*,
//             readability-avoid-const-params-in-decls)
using namespace Gui;
namespace sc = boost::statechart;
using SC = NavigationStateChart;
using NS = OnshapeNavigationStyle;

struct NS::NaviMachine: public sc::state_machine<NS::NaviMachine, NS::IdleState>
{
    using superclass = sc::state_machine<NS::NaviMachine, NS::IdleState>;
    explicit NaviMachine(NS& ns)
        : ns(ns)
    {}
    NS& ns;
};

struct NS::IdleState: public sc::state<NS::IdleState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<SC::Event>;
    explicit IdleState(my_context ctx)
        : my_base(ctx)
    {
        auto& ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::IDLE);
    }
    sc::result react(const SC::Event& ev)
    {
        auto& ns = this->outermost_context().ns;
        switch (ns.getViewingMode()) {
            case NavigationStyle::SEEK_WAIT_MODE: {
                if (ev.isPress(SoMouseButtonEvent::BUTTON1)) {
                    ns.seekToPoint(ev.inventor_event->getPosition());
                    ns.setViewingMode(NavigationStyle::SEEK_MODE);
                    ev.flags->processed = true;
                    return transit<NS::AwaitingReleaseState>();
                }
                break;
            }
            case NavigationStyle::SPINNING:
            case NavigationStyle::SEEK_MODE: {
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

        if (ev.isPress(SoMouseButtonEvent::BUTTON2)) {
            ev.flags->processed = true;
            if (ev.isDownControl()) {
                return transit<NS::AwaitingPanState>();
            }
            return transit<NS::AwaitingMoveState>();
        }

        if (ev.isPress(SoMouseButtonEvent::BUTTON3)) {
            ev.flags->processed = true;
            return transit<NS::AwaitingPanState>();
        }

        if (ev.isLocation2Event() && ev.isDownButton1()) {
            ev.flags->processed = true;
            return transit<NS::SelectionState>();
        }

        return forward_event();
    }
};

struct NS::AwaitingReleaseState: public sc::state<NS::AwaitingReleaseState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;
    explicit AwaitingReleaseState(my_context ctx)
        : my_base(ctx)
    {}

    sc::result react(const NS::Event& /*ev*/)
    {
        return forward_event();
    }
};

struct NS::InteractState: public sc::state<NS::InteractState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;
    explicit InteractState(my_context ctx)
        : my_base(ctx)
    {
        auto& ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::INTERACT);
    }

    sc::result react(const NS::Event& /*ev*/)
    {
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
    explicit AwaitingMoveState(my_context ctx)
        : my_base(ctx)
    {
        this->base_pos
            = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        this->since
            = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getTime();
    }
    sc::result react(const NS::Event& ev)
    {
        auto& ns = this->outermost_context().ns;
        // this state consumes all mouse events.
        ev.flags->processed = ev.isMouseButtonEvent() || ev.isLocation2Event();

        if (ev.isLocation2Event()) {
            if (ev.isDownButton3()) {
                return transit<NS::PanState>();
            }

            if (ev.isDownControl() && ev.isDownButton2()) {
                return transit<NS::PanState>();
            }

            if (ev.isDownButton2()) {
                return transit<NS::RotateState>();
            }
        }

        // right-click
        if (ev.isRelease(SoMouseButtonEvent::BUTTON2) && ev.mbstate() == 0
            && !ns.viewer->isEditing() && ns.isPopupMenuEnabled()) {
            ns.openPopupMenu(ev.inventor_event->getPosition());
            return transit<NS::IdleState>();
        }

        return forward_event();
    }
};

struct NS::AwaitingPanState: public sc::state<NS::AwaitingPanState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;

private:
    SbVec2s base_pos;
    SbTime since;

public:
    explicit AwaitingPanState(my_context ctx)
        : my_base(ctx)
    {
        auto& ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::PANNING);
        this->base_pos
            = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        this->since
            = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getTime();
    }
    sc::result react(const NS::Event& ev)
    {
        // this state consumes all mouse events.
        ev.flags->processed = ev.isMouseButtonEvent() || ev.isLocation2Event();

        if (ev.isLocation2Event()) {
            if (ev.isDownButton3()) {
                return transit<NS::PanState>();
            }

            if (ev.isDownControl() && ev.isDownButton2()) {
                return transit<NS::PanState>();
            }

            if (ev.isDownButton2()) {
                return transit<NS::RotateState>();
            }
        }

        // middle-click
        if (ev.isRelease(SoMouseButtonEvent::BUTTON3) && ev.isDownNoButton()) {
            auto& ns = this->outermost_context().ns;
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
    explicit RotateState(my_context ctx)
        : my_base(ctx)
    {
        auto& ns = this->outermost_context().ns;
        const auto inventorEvent
            = static_cast<const NS::Event*>(this->triggering_event())->inventor_event;
        ns.saveCursorPosition(inventorEvent);
        ns.setViewingMode(NavigationStyle::DRAGGING);
        this->base_pos = inventorEvent->getPosition();
    }

    sc::result react(const NS::Event& ev)
    {
        if (ev.isLocation2Event() && ev.isDownButton2()) {
            auto& ns = this->outermost_context().ns;
            ns.addToLog(ev.inventor_event->getPosition(), ev.inventor_event->getTime());
            const SbVec2s pos = ev.inventor_event->getPosition();
            const SbVec2f posn = ns.normalizePixelPos(pos);
            ns.spin(posn);
            ns.moveCursorPosition();
            ev.flags->processed = true;
        }

        if (ev.isKeyPress(SoKeyboardEvent::LEFT_CONTROL) && ev.isDownButton2()) {
            ev.flags->processed = true;
            return transit<NS::AwaitingPanState>();
        }

        if (ev.isRelease(SoMouseButtonEvent::BUTTON2) && ev.isDownNoButton()) {
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
    explicit PanState(my_context ctx)
        : my_base(ctx)
    {
        auto& ns = this->outermost_context().ns;
        const NS::Event* ev = static_cast<const NS::Event*>(this->triggering_event());
        ns.setViewingMode(NavigationStyle::PANNING);
        this->base_pos = ev->inventor_event->getPosition();
        this->ratio = ns.viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
        ns.centerTime = ev->inventor_event->getTime();
        ns.setupPanningPlane(ns.getCamera());
    }
    sc::result react(const NS::Event& ev)
    {
        if (ev.isLocation2Event()) {
            ev.flags->processed = true;
            SbVec2s pos = ev.inventor_event->getPosition();
            auto& ns = this->outermost_context().ns;
            ns.panCamera(
                ns.viewer->getSoRenderManager()->getCamera(),
                this->ratio,
                ns.panningplane,
                ns.normalizePixelPos(pos),
                ns.normalizePixelPos(this->base_pos)
            );
            this->base_pos = pos;
        }

        if (ev.isKeyRelease(SoKeyboardEvent::LEFT_CONTROL) && ev.isDownButton2()) {
            ev.flags->processed = true;
            return transit<NS::RotateState>();
        }

        if (ev.isKeyRelease(SoKeyboardEvent::LEFT_CONTROL)
            || ev.isRelease(SoMouseButtonEvent::BUTTON2)) {
            ev.flags->processed = true;
            return transit<NS::IdleState>();
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
    explicit ZoomState(my_context ctx)
        : my_base(ctx)
    {
        auto& ns = this->outermost_context().ns;
        const NS::Event* ev = static_cast<const NS::Event*>(this->triggering_event());
        ns.setViewingMode(NavigationStyle::ZOOMING);
        this->base_pos = ev->inventor_event->getPosition();
    }

    sc::result react(const NS::Event& ev)
    {
        if (ev.isLocation2Event()) {
            ev.flags->processed = true;
            SbVec2s pos = ev.inventor_event->getPosition();
            auto& ns = this->outermost_context().ns;
            ns.zoomByCursor(ns.normalizePixelPos(pos), ns.normalizePixelPos(this->base_pos));
            this->base_pos = pos;
        }

        return forward_event();
    }

private:
    SbVec2s base_pos;
};

struct NS::SelectionState: public sc::state<NS::SelectionState, NS::NaviMachine>
{
    using reactions = sc::custom_reaction<NS::Event>;
    explicit SelectionState(my_context ctx)
        : my_base(ctx)
    {
        auto& ns = this->outermost_context().ns;
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

        auto& ns = this->outermost_context().ns;
        ns.addSelectionCallback();
        ns.processEvent(&mbe);
    }

    sc::result react(const NS::Event& /*ev*/)
    {
        // This isn't called while selection mode is active
        return transit<NS::IdleState>();
    }
};

// ----------------------------------------------------------------------------------

// See https://forum.freecad.org/viewtopic.php?t=104415
// Left mouse: Select
// Left mouse drag: Box selection
// Right mouse drag: Rotate
// Middle mouse drag = Pan
// Ctrl + right mouse drag: Pan
// Scroll wheel forward: Zoom in
// Scroll wheel backward: Zoom out

/* TRANSLATOR Gui::OnshapeNavigationStyle */

TYPESYSTEM_SOURCE(Gui::OnshapeNavigationStyle, Gui::UserNavigationStyle)

OnshapeNavigationStyle::OnshapeNavigationStyle()
{
    naviMachine.reset(new NaviStateMachineT(new NaviMachine(*this)));
}

OnshapeNavigationStyle::~OnshapeNavigationStyle()
{}

const char* OnshapeNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
        case NavigationStyle::SELECTION:
            return QT_TR_NOOP("Press left mouse button");
        case NavigationStyle::PANNING:
            return QT_TR_NOOP("Press middle mouse button");
        case NavigationStyle::DRAGGING:
            return QT_TR_NOOP("Press right mouse button");
        case NavigationStyle::ZOOMING:
            return QT_TR_NOOP("Scroll mouse wheel");
        default:
            return "No description";
    }
}

std::string OnshapeNavigationStyle::userFriendlyName() const
{
    return {"Onshape"};
}

SbBool OnshapeNavigationStyle::processKeyboardEvent(const SoKeyboardEvent* const event)
{
    switch (event->getKey()) {
        case SoKeyboardEvent::Z:
            if (event->wasShiftDown()) {
                zoomIn();
            }
            else {
                zoomOut();
            }
            return true;
        default:
            break;
    }

    return inherited::processKeyboardEvent(event);
}

// NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast,
//           cppcoreguidelines-avoid*,
//           readability-avoid-const-params-in-decls)
