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
#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/state.hpp>

#include "Camera.h"
#include "NavigationStateChart.h"
#include "View3DInventorViewer.h"


using namespace Gui;
namespace sc = boost::statechart;
using NS = NavigationStateChart;

NS::Event::Event() : flags(new Flags)
{}

bool NS::Event::isMouseButtonEvent() const
{
    return this->inventor_event->isOfType(SoMouseButtonEvent::getClassTypeId());
}

const SoMouseButtonEvent* NS::Event::asMouseButtonEvent() const
{
    return static_cast<const SoMouseButtonEvent*>(this->inventor_event);
}

bool NS::Event::isPress(Button button) const
{
    return SoMouseButtonEvent::isButtonPressEvent(this->inventor_event, button);
}

bool NS::Event::isRelease(Button button) const
{
    return SoMouseButtonEvent::isButtonReleaseEvent(this->inventor_event, button);
}

bool NS::Event::isKeyPress(Key key) const
{
    return SoKeyboardEvent::isKeyPressEvent(this->inventor_event, key);
}

bool NS::Event::isKeyRelease(Key key) const
{
    return SoKeyboardEvent::isKeyReleaseEvent(this->inventor_event, key);
}

bool NS::Event::isKeyboardEvent() const
{
    return this->inventor_event->isOfType(SoKeyboardEvent::getClassTypeId());
}

const SoKeyboardEvent* NS::Event::asKeyboardEvent() const
{
    return static_cast<const SoKeyboardEvent*>(this->inventor_event);
}

bool NS::Event::isLocation2Event() const
{
    return this->inventor_event->isOfType(SoLocation2Event::getClassTypeId());
}

const SoLocation2Event* NS::Event::asLocation2Event() const
{
    return static_cast<const SoLocation2Event*>(this->inventor_event);
}

bool NS::Event::isMotion3Event() const
{
    return this->inventor_event->isOfType(SoMotion3Event::getClassTypeId());
}

const SoMotion3Event* NS::Event::asMotion3Event() const
{
    return static_cast<const SoMotion3Event*>(this->inventor_event);
}

bool NS::Event::isDownButton(unsigned int state) const
{
    return mbstate() == state;
}

bool NS::Event::isDownNoButton() const
{
    return mbstate() == 0;
}

bool NS::Event::isDownButton1() const
{
    return (mbstate() & BUTTON1DOWN) == BUTTON1DOWN;
}

bool NS::Event::isDownButton2() const
{
    return (mbstate() & BUTTON2DOWN) == BUTTON2DOWN;
}

bool NS::Event::isDownButton3() const
{
    return (mbstate() & BUTTON3DOWN) == BUTTON3DOWN;
}

bool NS::Event::isDownControl() const
{
    return (kbstate() & CTRLDOWN) == CTRLDOWN;
}

bool NS::Event::isDownShift() const
{
    return (kbstate() & SHIFTDOWN) == SHIFTDOWN;
}

bool NS::Event::isDownAlt() const
{
    return (kbstate() & ALTDOWN) == ALTDOWN;
}


/* TRANSLATOR Gui::NavigationStateChart */

TYPESYSTEM_SOURCE_ABSTRACT(Gui::NavigationStateChart, Gui::UserNavigationStyle)

NavigationStateChart::NavigationStateChart()
{}

NavigationStateChart::~NavigationStateChart()
{
    naviMachine.reset();
}

SbBool NavigationStateChart::processSoEvent(const SoEvent * const ev)
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
