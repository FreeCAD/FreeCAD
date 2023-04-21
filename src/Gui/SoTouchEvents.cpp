/***************************************************************************
 *   Copyright (c) 2015 Victor Titov (DeepSOIC) <vv.titov@gmail.com)>      *
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

#include <QApplication>
#include <QGestureEvent>
#include <QWidget>

#include <Base/Exception.h>

#include "SoTouchEvents.h"


SO_EVENT_SOURCE(SoGestureEvent);

SbBool SoGestureEvent::isSoGestureEvent(const SoEvent *ev) const
{
    return ev->isOfType(SoGestureEvent::getClassTypeId());
}

//----------------------------SoGesturePanEvent--------------------------------

SO_EVENT_SOURCE(SoGesturePanEvent);

SoGesturePanEvent::SoGesturePanEvent(QPanGesture* qpan, QWidget *widget)
{
    Q_UNUSED(widget);
    totalOffset = SbVec2f(qpan->offset().x(), -qpan->offset().y());
    deltaOffset = SbVec2f(qpan->delta().x(), -qpan->delta().y());
    state = SbGestureState(qpan->state());

    Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
    this->setAltDown(mods.testFlag(Qt::AltModifier));
    this->setCtrlDown(mods.testFlag(Qt::ControlModifier));
    this->setShiftDown(mods.testFlag(Qt::ShiftModifier));
    this->setTime(SbTime::getTimeOfDay());
}

SbBool SoGesturePanEvent::isSoGesturePanEvent(const SoEvent *ev) const
{
    return ev->isOfType(SoGesturePanEvent::getClassTypeId());
}

//----------------------------SoGesturePinchEvent--------------------------------

SO_EVENT_SOURCE(SoGesturePinchEvent);

SoGesturePinchEvent::SoGesturePinchEvent(QPinchGesture* qpinch, QWidget *widget)
{
    int h = widget->height();
    QPointF widgetCorner = QPointF(widget->mapToGlobal(QPoint(0,0)));
    qreal scaleToWidget = (widget->mapFromGlobal(QPoint(800,800))-widget->mapFromGlobal(QPoint(0,0))).x()/800.0;
    QPointF pnt;//temporary
    pnt = qpinch->startCenterPoint();
    pnt = (pnt-widgetCorner)*scaleToWidget;//translate screen coord. into widget coord.
    startCenter = SbVec2f(pnt.x(), h - pnt.y());

    pnt = qpinch->centerPoint();
    pnt = (pnt-widgetCorner)*scaleToWidget;
    curCenter = SbVec2f(pnt.x(), h - pnt.y());

    pnt = qpinch->lastCenterPoint();
    pnt = (pnt-widgetCorner)*scaleToWidget;
    deltaCenter = curCenter - SbVec2f(pnt.x(), h - pnt.y());

    deltaZoom = qpinch->scaleFactor();
    totalZoom = qpinch->totalScaleFactor();

    deltaAngle = -unbranchAngle((qpinch->rotationAngle()-qpinch->lastRotationAngle()) / 180.0 * M_PI);
    totalAngle = -qpinch->totalRotationAngle() / 180 * M_PI;

    state = SbGestureState(qpinch->state());

    this->setPosition(SbVec2s(curCenter));
    Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
    this->setAltDown(mods.testFlag(Qt::AltModifier));
    this->setCtrlDown(mods.testFlag(Qt::ControlModifier));
    this->setShiftDown(mods.testFlag(Qt::ShiftModifier));
    this->setTime(SbTime::getTimeOfDay());
}

SbBool SoGesturePinchEvent::isSoGesturePinchEvent(const SoEvent *ev) const
{
    return ev->isOfType(SoGesturePinchEvent::getClassTypeId());
}

/*!
 * \brief SoGesturePinchEvent::unbranchAngle : utility function to bring an angle into -pi..pi region.
 * \param ang - in radians
 * \return
 */
double SoGesturePinchEvent::unbranchAngle(double ang)
{
    return ang - 2.0 * M_PI * floor((ang + M_PI) / (2.0 * M_PI));
}


//----------------------------SoGestureSwipeEvent--------------------------------

SO_EVENT_SOURCE(SoGestureSwipeEvent);

SoGestureSwipeEvent::SoGestureSwipeEvent(QSwipeGesture *qwsipe, QWidget *widget)
{
    Q_UNUSED(widget);
    angle = qwsipe->swipeAngle();
    switch (qwsipe->verticalDirection()){
    case QSwipeGesture::Up :
        vertDir = +1;
        break;
    case QSwipeGesture::Down :
        vertDir = -1;
        break;
    default:
        vertDir = 0;
        break;
    }
    switch (qwsipe->horizontalDirection()){
    case QSwipeGesture::Right :
        horzDir = +1;
        break;
    case QSwipeGesture::Left :
        horzDir = -1;
        break;
    default:
        horzDir = 0;
        break;
    }

    state = SbGestureState(qwsipe->state());

    Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
    this->setAltDown(mods.testFlag(Qt::AltModifier));
    this->setCtrlDown(mods.testFlag(Qt::ControlModifier));
    this->setShiftDown(mods.testFlag(Qt::ShiftModifier));
    this->setTime(SbTime::getTimeOfDay());
}

SbBool SoGestureSwipeEvent::isSoGestureSwipeEvent(const SoEvent *ev) const
{
    return ev->isOfType(SoGestureSwipeEvent::getClassTypeId());
}


//----------------------------GesturesDevice-------------------------------

GesturesDevice::GesturesDevice(QWidget* widget)
{
    if (SoGestureEvent::getClassTypeId().isBad()){
        SoGestureEvent::initClass();
        SoGesturePanEvent::initClass();
        SoGesturePinchEvent::initClass();
        SoGestureSwipeEvent::initClass();
    }
    if (! widget)
        throw Base::ValueError("Can't create a gestures quarter input device without widget (null pointer was passed).");
    this->widget = widget;
}

const SoEvent* GesturesDevice::translateEvent(QEvent* event)
{
    if (event->type() == QEvent::Gesture
            || event->type() == QEvent::GestureOverride) {
        auto gevent = static_cast<QGestureEvent*>(event);

        auto zg = static_cast<QPinchGesture*>(gevent->gesture(Qt::PinchGesture));
        if(zg){
            gevent->setAccepted(Qt::PinchGesture,true);//prefer it over pan
            return new SoGesturePinchEvent(zg,this->widget);
        }

        auto pg = static_cast<QPanGesture*>(gevent->gesture(Qt::PanGesture));
        if(pg){
            gevent->setAccepted(Qt::PanGesture,true);
            return new SoGesturePanEvent(pg,this->widget);
        }

        auto sg = static_cast<QSwipeGesture*>(gevent->gesture(Qt::SwipeGesture));
        if(sg){
            gevent->setAccepted(Qt::SwipeGesture,true);
            return new SoGesturePanEvent(pg,this->widget);
        }
    }
    return nullptr;
}
