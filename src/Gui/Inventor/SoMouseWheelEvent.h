/***************************************************************************
 *   Copyright (c) 2020 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

#ifndef SOMOUSEWHEELEVENT_H_FC
#define SOMOUSEWHEELEVENT_H_FC


#include <Inventor/events/SoEvent.h>
#include <Inventor/events/SoSubEvent.h>
#include <FCGlobal.h>

/**
 * @brief The SoMouseWheelEvent class is a temporary replacement for
 * SoMouseWheelEvent from Coin, for until freecad stops using Coin version that
 * doesn't have one (coin v 4.0.0a doesn't have SoMouseWheelEvent).
 */
class GuiExport SoMouseWheelEvent : public SoEvent {
    SO_EVENT_HEADER();
public: //methods
    static void initClass(){
        SO_EVENT_INIT_CLASS(SoMouseWheelEvent, SoEvent);
    }
    SoMouseWheelEvent () : delta(0) {}
    SoMouseWheelEvent (int delta) : delta(delta) {}
    ///returns wheel position change. One click is usually 120 units,
    /// smaller values come from high-resolution devices like touchpads
    int getDelta() const {return delta;}
    void setDelta(int delta) {this->delta = delta;}
    ~SoMouseWheelEvent() override = default;

private: //data
    int delta;
};

#endif
