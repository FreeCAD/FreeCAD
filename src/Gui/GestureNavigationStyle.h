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

#ifndef GESTURENAVIGATIONSTYLE2_H
#define GESTURENAVIGATIONSTYLE2_H

#include "NavigationStyle.h"

#include <queue>
#include <memory>

namespace Gui {


class GestureNavigationStyle: public UserNavigationStyle
{
    typedef UserNavigationStyle superclass;

    TYPESYSTEM_HEADER();

public:
    GestureNavigationStyle();
    virtual ~GestureNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;
public:
    ///calls processSoEvent of NavigationStyle.
    SbBool processSoEvent_bypass(const SoEvent* const ev);

protected://state machine classes
    ///State machine event, a wrapper around SoEvent
    class Event;

    class NaviMachine;

    class IdleState;
    ///when operating a dragger, for example
    class InteractState;
    ///button was pressed, but the cursor hasn't moved yet
    class AwaitingMoveState;
    ///when in a two-finger touchscreen gestures
    class GestureState;
    ///rotating the viewed model with mouse drag
    class RotateState;
    ///panning with mouse drag
    class PanState;
    ///panning triggered by tap-and-hold. It won't switch to Tilt and Rotate upon pressing/releasing mouse buttons
    class StickyPanState;
    ///tilting with mouse drag
    class TiltState;
    ///this state discards all mouse input (except detecting button roll gestures), until all buttons are released.
    class AwaitingReleaseState;

    class EventQueue: public std::queue<SoMouseButtonEvent>
    {
    public:
        EventQueue(GestureNavigationStyle& ns):ns(ns){}

        void post(const Event& ev);
        void discardAll();
        void forwardAll();
    public:
        GestureNavigationStyle& ns;
    };


protected: // members variables
    std::unique_ptr<NaviMachine> naviMachine;
    EventQueue postponedEvents;

    //settings:
    ///if false, tilting with touchscreen gestures will be disabled
    bool enableGestureTilt = false;
    ///distance in px to treat as a definite drag (noise gate)
    int mouseMoveThreshold = 5;
    ///used by roll gesture detection logic, in AwaitingMoveState and AwaitingReleaseState.
    int rollDir = 0;

protected: //helper functions
    bool isDraggerUnderCursor(SbVec2s pos);
public:
    bool is2DViewing() const;

public: //gesture reactions
    ///Roll gesture is like: press LMB, press RMB, release LMB, release RMB.
    /// This function is called by state machine whenever it picks up roll gesture.
    void onRollGesture(int direction);
    ///Called by state machine, when set-rotation-center gesture is detected (MMB click, or H key)
    void onSetRotationCenter(SbVec2s cursor);
};

}
#endif // GESTURENAVIGATIONSTYLE2_H
