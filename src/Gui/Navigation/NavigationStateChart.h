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


#pragma once

#include <boost/statechart/event.hpp>
#include <Gui/Navigation/NavigationInputState.h>
#include <Gui/Navigation/NavigationStyle.h>

// NOLINTBEGIN(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
namespace Gui
{

class NaviStateMachine;

class GuiExport NavigationStateChart: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    struct Event: public boost::statechart::event<Event>
    {
        using Button = SoMouseButtonEvent::Button;
        using Key = SoKeyboardEvent::Key;

        Event();
        bool isMouseButtonEvent() const;
        const SoMouseButtonEvent* asMouseButtonEvent() const;
        bool isPress(Button button) const;
        bool isRelease(Button button) const;
        bool isKeyPress(Key key) const;
        bool isKeyRelease(Key key) const;
        bool isKeyboardEvent() const;
        const SoKeyboardEvent* asKeyboardEvent() const;
        bool isLocation2Event() const;
        const SoLocation2Event* asLocation2Event() const;
        bool isMotion3Event() const;
        const SoMotion3Event* asMotion3Event() const;
        bool isDownButton(unsigned int state) const;
        bool isDownNoButton() const;
        bool isDownButton1() const;
        bool isDownButton2() const;
        bool isDownButton3() const;
        bool isDownControl() const;
        bool isDownShift() const;
        bool isDownAlt() const;

        enum
        {
            BUTTON1DOWN = NavigationInputState::LeftDown,
            BUTTON2DOWN = NavigationInputState::RightDown,
            BUTTON3DOWN = NavigationInputState::MiddleDown,
            CTRLDOWN = NavigationInputState::CtrlDown,
            SHIFTDOWN = NavigationInputState::ShiftDown,
            ALTDOWN = NavigationInputState::AltDown,
            MASKBUTTONS = NavigationInputState::ButtonMask,
            MASKMODIFIERS = NavigationInputState::ModifierMask
        };

        const SoEvent* inventor_event {nullptr};
        unsigned int modifiers {0};
        unsigned int mbstate() const
        {
            return modifiers & MASKBUTTONS;
        }
        unsigned int kbstate() const
        {
            return modifiers & MASKMODIFIERS;
        }
        [[nodiscard]] NavigationInputState inputState() const
        {
            return {
                .left = (modifiers & BUTTON1DOWN) != 0,
                .middle = (modifiers & BUTTON3DOWN) != 0,
                .right = (modifiers & BUTTON2DOWN) != 0,
                .ctrl = (modifiers & CTRLDOWN) != 0,
                .shift = (modifiers & SHIFTDOWN) != 0,
                .alt = (modifiers & ALTDOWN) != 0
            };
        }

        struct Flags
        {
            bool processed = false;
            bool propagated = false;
        };
        std::shared_ptr<Flags> flags;
    };

    NavigationStateChart();
    ~NavigationStateChart() override;

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;
    std::unique_ptr<NaviStateMachine> naviMachine;  // NOLINT
};

class GuiExport NaviStateMachine
{
public:
    NaviStateMachine(const NaviStateMachine&) = delete;
    NaviStateMachine(NaviStateMachine&&) = delete;
    NaviStateMachine& operator=(const NaviStateMachine&) = delete;
    NaviStateMachine& operator=(NaviStateMachine&&) = delete;

    NaviStateMachine() = default;
    virtual ~NaviStateMachine() = default;

    virtual void process_event(const NavigationStateChart::Event&) = 0;
};

template<typename T>
class NaviStateMachineT: public NaviStateMachine
{
public:
    explicit NaviStateMachineT(T* t)
        : object(t)
    {
        object->initiate();
    }

    ~NaviStateMachineT() override
    {
        object.reset();
    }

    void process_event(const NavigationStateChart::Event& ev) override
    {
        object->process_event(ev);
    }

private:
    std::unique_ptr<T> object;
};

}  // namespace Gui
// NOLINTEND(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
