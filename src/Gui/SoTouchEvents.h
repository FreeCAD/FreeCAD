/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)                                 *
 *                                           (vv.titov@gmail.com) 2015     *
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

#ifndef SOTOUCHEVENTS_H
#define SOTOUCHEVENTS_H

#include <QGesture>
#include <Quarter/devices/InputDevice.h>
#include <Inventor/SbLinear.h>
#include <Inventor/events/SoEvent.h>
#include <Inventor/events/SoSubEvent.h>

class QWidget;

namespace Quarter = SIM::Coin3D::Quarter;

class SoGestureEvent : public SoEvent {
    SO_EVENT_HEADER();
public:
    static void initClass(){
        SO_EVENT_INIT_CLASS(SoGestureEvent, SoEvent);
    }
    SoGestureEvent() : state(SbGSNoGesture) {}
    ~SoGestureEvent(){}
    SbBool isSoGestureEvent(const SoEvent* ev) const;

    enum SbGestureState {
        SbGSNoGesture = Qt::NoGesture,
        SbGSStart = Qt::GestureStarted,
        SbGSUpdate = Qt::GestureUpdated,
        SbGSEnd = Qt::GestureFinished,
        SbGsCanceled = Qt::GestureCanceled
    };
    SbGestureState state;
};

class SoGesturePanEvent : public SoGestureEvent {
    SO_EVENT_HEADER();
public:
    static void initClass(){//needs to be called before the class can be used. Initializes type IDs of the class.
        SO_EVENT_INIT_CLASS(SoGesturePanEvent, SoGestureEvent);
    }
    SoGesturePanEvent() {}
    SoGesturePanEvent(QPanGesture *qpan, QWidget *widget);
    ~SoGesturePanEvent(){}
    SbBool isSoGesturePanEvent(const SoEvent* ev) const;

    SbVec2f deltaOffset;
    SbVec2f totalOffset;
};

class SoGesturePinchEvent : public SoGestureEvent {
    SO_EVENT_HEADER();
public:
    static void initClass(){
        SO_EVENT_INIT_CLASS(SoGesturePinchEvent, SoGestureEvent);
    }
    SoGesturePinchEvent() : deltaZoom(0), totalZoom(0),
                            deltaAngle(0), totalAngle(0)
    {
    }
    SoGesturePinchEvent(QPinchGesture* qpinch, QWidget* widget);
    ~SoGesturePinchEvent(){}
    SbBool isSoGesturePinchEvent(const SoEvent* ev) const;

    SbVec2f startCenter;//in GL pixel coordinates (from bottom left corner of view area)
    SbVec2f curCenter;
    SbVec2f deltaCenter;
    double deltaZoom;//change of zoom factor (1.0 = no change, >1 - zoom in, 0..1 - zoom out)
    double totalZoom;//zoom factor accumulated since start of gesture.
    double deltaAngle;
    double totalAngle;

    static double unbranchAngle(double ang);

};

class SoGestureSwipeEvent : public SoGestureEvent {
    SO_EVENT_HEADER();
public:
    static void initClass(){
        SO_EVENT_INIT_CLASS(SoGestureSwipeEvent, SoGestureEvent);
    }
    SoGestureSwipeEvent() : angle(0), vertDir(0), horzDir(0)
    {
    }
    SoGestureSwipeEvent(QSwipeGesture* qwsipe, QWidget *widget);
    ~SoGestureSwipeEvent(){}
    SbBool isSoGestureSwipeEvent(const SoEvent* ev) const;

    double angle;
    int vertDir;//+1,0,-1 up/none/down
    int horzDir;//+1,0,-1 right/none/left
};


class GesturesDevice : public Quarter::InputDevice {
public:
    GesturesDevice(QWidget* widget);//it needs to know the widget to do coordinate translation

    virtual ~GesturesDevice()  {}
    virtual const SoEvent* translateEvent(QEvent* event);
protected:
    QWidget* widget;
};

#endif // SOTOUCHEVENTS_H
