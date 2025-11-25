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


#include "SpaceballEvent.h"


namespace Spaceball
{

int MotionEvent::MotionEventType = -1;
int ButtonEvent::ButtonEventType = -1;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
EventBase::EventBase(QEvent::Type event)
    : QInputEvent(static_cast<QEvent::Type>(event))
#else
EventBase::EventBase(QEvent::Type event)
    : QInputEvent(static_cast<QEvent::Type>(event), QPointingDevice::primaryPointingDevice())
#endif
{
}

MotionEvent::MotionEvent()
    : EventBase(static_cast<QEvent::Type>(MotionEventType))
{
}

ButtonEvent::ButtonEvent()
    : EventBase(static_cast<QEvent::Type>(ButtonEventType))
{
}

} // namespace Spaceball

