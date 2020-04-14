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
namespace Spaceball
{
    enum ButtonStateType {BUTTON_NONE = 0, BUTTON_PRESSED, BUTTON_RELEASED};

    class EventBase : public QInputEvent
    {
    public:
        bool isHandled(){return handled;}
        void setHandled(bool sig){handled = sig;}

    protected:
        EventBase(QEvent::Type event);
        bool handled;
    };

    class MotionEvent : public EventBase
    {
    public:
        MotionEvent();
        MotionEvent(const MotionEvent& in);
        void translations(int &xTransOut, int &yTransOut, int &zTransOut);
        void setTranslations(const int &xTransIn, const int &yTransIn, const int &zTransIn);
        int translationX(){return xTrans;}
        int translationY(){return yTrans;}
        int translationZ(){return zTrans;}

        void rotations(int &xRotOut, int &yRotOut, int &zRotOut);
        void setRotations(const int &xRotIn, const int &yRotIn, const int &zRotIn);
        int rotationX(){return xRot;}
        int rotationY(){return yRot;}
        int rotationZ(){return zRot;}

        static int MotionEventType;

    private:
        int xTrans;
        int yTrans;
        int zTrans;
        int xRot;
        int yRot;
        int zRot;
    };

    class ButtonEvent : public EventBase
    {
    public:
        ButtonEvent();
        ButtonEvent(const ButtonEvent& in);
        ButtonStateType buttonStatus();
        void setButtonStatus(const ButtonStateType &buttonStatusIn);
        int buttonNumber();
        void setButtonNumber(const int &buttonNumberIn);

        static int ButtonEventType;

    private:
        ButtonStateType buttonState;
        int button;
    };
}
#endif // SPACEBALLEVENT_H
