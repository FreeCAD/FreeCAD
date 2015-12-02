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

/*! This file adds support for pinch gestures in Windows 8 to Qt4.8. I think it
 * may not be necessary for Qt5. I also think this was actually not absolutely
 * necessary, and it may be possible to force Qt gesture recognition from plain
 * touch input.          --DeepSOIC
 */

#ifndef WINNATIVEGESTURERECOGNIZERS_H
#define WINNATIVEGESTURERECOGNIZERS_H

#include <QGestureRecognizer>
#include <QPinchGesture>

#ifdef Q_WS_WIN
#if QT_VERSION < 0x050000
#if(WINVER >= 0x0601) // need Windows 7
#define GESTURE_MESS
#endif
#endif // QT_VERSION < 0x050000
#endif // Q_WS_WIN

#ifdef GESTURE_MESS

/*!
 * \brief The QPinchGestureN class is a special version of QPinchGesture,
 * containing a few extra fields for state tracking.
 */
class QPinchGestureN: public QPinchGesture
{
public:
    int lastFingerDistance;//distance between fingers, in pixels
    int fingerDistance;    
    double myRotationAngle;
    double myLastRotationAngle;
};

class WinNativeGestureRecognizerPinch : public QGestureRecognizer
{
public:
    WinNativeGestureRecognizerPinch(){}
    virtual QGesture* create ( QObject* target );
    virtual Result recognize ( QGesture* gesture, QObject* watched, QEvent* event );
    virtual void reset ( QGesture* gesture );
    static void TuneWindowsGestures(QWidget* target);
    static double unbranchAngle(double ang);
};

#endif //GESTUREMESS

#endif // WINNATIVEGESTURERECOGNIZERS_H
