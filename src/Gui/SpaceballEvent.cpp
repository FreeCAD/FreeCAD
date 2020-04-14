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

#include "PreCompiled.h"
#include "SpaceballEvent.h"
#include "Application.h"

using namespace Spaceball;

int MotionEvent::MotionEventType = -1;
int ButtonEvent::ButtonEventType = -1;

EventBase::EventBase(QEvent::Type event) : QInputEvent(static_cast<QEvent::Type>(event)), handled(false)
{

}

MotionEvent::MotionEvent() : EventBase(static_cast<QEvent::Type>(MotionEventType)),
    xTrans(0), yTrans(0), zTrans(0), xRot(0), yRot(0), zRot(0)
{
}

MotionEvent::MotionEvent(const MotionEvent& in) : EventBase(static_cast<QEvent::Type>(MotionEventType))
{
    xTrans  = in.xTrans;
    yTrans  = in.yTrans;
    zTrans  = in.zTrans;
    xRot    = in.xRot;
    yRot    = in.yRot;
    zRot    = in.zRot;
    handled = in.handled;
}

void MotionEvent::translations(int &xTransOut, int &yTransOut, int &zTransOut)
{
    xTransOut = xTrans;
    yTransOut = yTrans;
    zTransOut = zTrans;
}

void MotionEvent::setTranslations(const int &xTransIn, const int &yTransIn, const int &zTransIn)
{
    xTrans = xTransIn;
    yTrans = yTransIn;
    zTrans = zTransIn;
}

void MotionEvent::rotations(int &xRotOut, int &yRotOut, int &zRotOut)
{
    xRotOut = xRot;
    yRotOut = yRot;
    zRotOut = zRot;
}

void MotionEvent::setRotations(const int &xRotIn, const int &yRotIn, const int &zRotIn)
{
    xRot = xRotIn;
    yRot = yRotIn;
    zRot = zRotIn;
}


ButtonEvent::ButtonEvent() : EventBase(static_cast<QEvent::Type>(ButtonEventType)),
    buttonState(BUTTON_NONE), button(0)
{
}

ButtonEvent::ButtonEvent(const ButtonEvent& in) : EventBase(static_cast<QEvent::Type>(ButtonEventType))
{
    buttonState = in.buttonState;
    button = in.button;
    handled = in.handled;
}

ButtonStateType ButtonEvent::buttonStatus()
{
    return buttonState;
}

void ButtonEvent::setButtonStatus(const ButtonStateType &buttonStatusIn)
{
    buttonState = buttonStatusIn;
}

int ButtonEvent::buttonNumber()
{
    return button;
}

void ButtonEvent::setButtonNumber(const int &buttonNumberIn)
{
    button = buttonNumberIn;
}
