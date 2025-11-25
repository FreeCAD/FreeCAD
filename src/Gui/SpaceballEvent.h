/***************************************************************************
 *   Copyright (c) 2010 Thomas Anderson <ta@nextgenengineering>            *
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

#ifndef SPACEBALLEVENT_H
#define SPACEBALLEVENT_H

#include <QInputEvent>

#include <tuple>

namespace Spaceball
{

enum class ButtonState
{
    None = 0,
    Pressed,
    Released
};

/**
 * @brief Base class for Spaceball events.
 */
class EventBase: public QInputEvent
{
public:
    [[nodiscard]] bool isHandled() const
    {
        return handled;
    }
    void setHandled(bool sig)
    {
        handled = sig;
    }

protected:
    explicit EventBase(QEvent::Type event);

private:
    bool handled{false};
};

/**
 * @brief Represents a motion event from the Spaceball.
 */
class MotionEvent: public EventBase
{
public:
    MotionEvent();
    MotionEvent(const MotionEvent& in) = default;
    MotionEvent& operator=(const MotionEvent& in) = default;
    MotionEvent(MotionEvent&&) noexcept = default;
    MotionEvent& operator=(MotionEvent&&) noexcept = default;
    ~MotionEvent() override = default;


    [[nodiscard]] std::tuple<int, int, int> translations() const
    {
        return {xTrans, yTrans, zTrans};
    }
    void setTranslations(int xTransIn, int yTransIn, int zTransIn)
    {
        xTrans = xTransIn;
        yTrans = yTransIn;
        zTrans = zTransIn;
    }

    [[nodiscard]] int translationX() const { return xTrans; }
    [[nodiscard]] int translationY() const { return yTrans; }
    [[nodiscard]] int translationZ() const { return zTrans; }

    [[nodiscard]] std::tuple<int, int, int> rotations() const
    {
        return {xRot, yRot, zRot};
    }
    void setRotations(int xRotIn, int yRotIn, int zRotIn)
    {
        xRot = xRotIn;
        yRot = yRotIn;
        zRot = zRotIn;
    }
    [[nodiscard]] int rotationX() const { return xRot; }
    [[nodiscard]] int rotationY() const { return yRot; }
    [[nodiscard]] int rotationZ() const { return zRot; }

    static int MotionEventType;

private:
    int xTrans {0};
    int yTrans {0};
    int zTrans {0};
    int xRot {0};
    int yRot {0};
    int zRot {0};
};

/**
 * @brief Represents a button event from the Spaceball.
 */
class ButtonEvent: public EventBase
{
public:
    ButtonEvent();
    ButtonEvent(const ButtonEvent& in) = default;
    ButtonEvent& operator=(const ButtonEvent& in) = default;
    ButtonEvent(ButtonEvent&&) noexcept = default;
    ButtonEvent& operator=(ButtonEvent&&) noexcept = default;
    ~ButtonEvent() override = default;


    [[nodiscard]] ButtonState buttonStatus() const
    {
        return buttonState;
    }
    void setButtonStatus(ButtonState buttonStatusIn)
    {
        buttonState = buttonStatusIn;
    }
    [[nodiscard]] int buttonNumber() const
    {
        return button;
    }
    void setButtonNumber(int buttonNumberIn)
    {
        button = buttonNumberIn;
    }

    static int ButtonEventType;

private:
    ButtonState buttonState{ButtonState::None};
    int button{0};
};

}  // namespace Spaceball

#endif  // SPACEBALLEVENT_H
