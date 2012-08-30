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
    int motionDataArray[6] = {in.xTrans, in.yTrans, in.zTrans, in.xRot, in.yRot, in.zRot};
    importSettings(motionDataArray);
    handled = in.handled;
}

float MotionEvent::convertPrefToSensitivity(int value)
{
    if (value < 0)
    {
        return ((0.6/50)*float(value) + 1);
    }
    else
    {
        return ((1.1/50)*float(value) + 1);
    }
}

void MotionEvent::importSettings(int* motionDataArray)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Spaceball")->GetGroup("Motion");
    
    // here I import settings from a dialog. For now they are set as is
    bool  dominant           = group->GetBool("Dominant"); // Is dominant checked
    bool  flipXY             = group->GetBool("FlipYZ");; // Is Flip X/Y checked
    float generalSensitivity = convertPrefToSensitivity(group->GetInt("GlobalSensitivity"));
  
    // array that has stored info about "Enabled" checkboxes of all axes    
    bool enabled[6];
    enabled[0] = group->GetBool("Translations", true) && group->GetBool("PanLREnable", true);
    enabled[1] = group->GetBool("Translations", true) && group->GetBool("PanUDEnable", true);
    enabled[2] = group->GetBool("Translations", true) && group->GetBool("ZoomEnable", true);
    enabled[3] = group->GetBool("Rotations", true) && group->GetBool("TiltEnable", true);
    enabled[4] = group->GetBool("Rotations", true) && group->GetBool("RollEnable", true);
    enabled[5] = group->GetBool("Rotations", true) && group->GetBool("SpinEnable", true);
    
    // array that has stored info about "Reversed" checkboxes of all axes 
    bool  reversed[6];
    reversed[0] = group->GetBool("PanLRReverse");
    reversed[1] = group->GetBool("PanUDReverse");
    reversed[2] = group->GetBool("ZoomReverse");
    reversed[3] = group->GetBool("TiltReverse");
    reversed[4] = group->GetBool("RollReverse");
    reversed[5] = group->GetBool("SpinReverse");

    // array that has stored info about sliders - on each slider you need to use method DlgSpaceballSettings::GetValuefromSlider
    // which will convert <-50, 50> linear integers from slider to <0.1, 10> exponential floating values 
    float sensitivity[6];
    sensitivity[0] = convertPrefToSensitivity(group->GetInt("PanLRSensitivity"));
    sensitivity[1] = convertPrefToSensitivity(group->GetInt("PanUDSensitivity"));
    sensitivity[2] = convertPrefToSensitivity(group->GetInt("ZoomSensitivity"));
    sensitivity[3] = convertPrefToSensitivity(group->GetInt("TiltSensitivity"));
    sensitivity[4] = convertPrefToSensitivity(group->GetInt("RollSensitivity"));
    sensitivity[5] = convertPrefToSensitivity(group->GetInt("SpinSensitivity"));
    
    int i;

    if (group->GetBool("Calibrate"))
    {
        group->SetInt("CalibrationX",motionDataArray[0]);
        group->SetInt("CalibrationY",motionDataArray[1]);
        group->SetInt("CalibrationZ",motionDataArray[2]);
        group->SetInt("CalibrationXr",motionDataArray[3]);
        group->SetInt("CalibrationYr",motionDataArray[4]);
        group->SetInt("CalibrationZr",motionDataArray[5]);

        group->RemoveBool("Calibrate");

        return;
    }
    else
    {
        motionDataArray[0] = motionDataArray[0] - group->GetInt("CalibrationX");
        motionDataArray[1] = motionDataArray[1] - group->GetInt("CalibrationY");
        motionDataArray[2] = motionDataArray[2] - group->GetInt("CalibrationZ");
        motionDataArray[3] = motionDataArray[3] - group->GetInt("CalibrationXr");
        motionDataArray[4] = motionDataArray[4] - group->GetInt("CalibrationYr");
        motionDataArray[5] = motionDataArray[5] - group->GetInt("CalibrationZr");        
    }
    
    if (dominant) { // if dominant is checked
        int max = 0;
        bool flag = false;
        for (i = 0; i < 6; ++i) {
            if (abs(motionDataArray[i]) > abs(max)) max = motionDataArray[i];
        }
        for (i = 0; i < 6; ++i) {
            if ((motionDataArray[i] != max) || (flag)) {
                motionDataArray[i] = 0;
            } else if (motionDataArray[i] == max){
                flag = true;
            }
        }
    }

    if (flipXY) {
        int temp = motionDataArray[1];
        motionDataArray[1] = motionDataArray[2];
        motionDataArray[2] = - temp;
    }
    
    for (i = 0; i < 6; ++i) {
        if (motionDataArray[i] != 0) {
            if (enabled[i] == false) 
                motionDataArray[i] = 0;
            else { 
                if (reversed[i] == true) 
                    motionDataArray[i] = - motionDataArray[i];
                motionDataArray[i] = (int)((float)(motionDataArray[i]) * sensitivity[i] * generalSensitivity);
            }
        }
    }

    xTrans  = motionDataArray[0];
    yTrans  = motionDataArray[1];
    zTrans  = motionDataArray[2];
    xRot    = motionDataArray[3];
    yRot    = motionDataArray[4];
    zRot    = motionDataArray[5];
}


void MotionEvent::setMotionData(int &xTransIn, int &yTransIn, int &zTransIn, int &xRotIn, int &yRotIn, int &zRotIn){
    int motionDataArray[6] = {xTransIn, yTransIn, zTransIn, xRotIn, yRotIn, zRotIn};
    importSettings(motionDataArray);
}

void MotionEvent::translations(int &xTransOut, int &yTransOut, int &zTransOut)
{
    xTransOut = xTrans;
    yTransOut = yTrans;
    zTransOut = zTrans;
}

void MotionEvent::setTranslations(const int &xTransIn, const int &yTransIn, const int &zTransIn)
{
    int motionDataArray[6] = {xTransIn, yTransIn, zTransIn, xRot, yRot, zRot};
    importSettings(motionDataArray);
}

void MotionEvent::rotations(int &xRotOut, int &yRotOut, int &zRotOut)
{
    xRotOut = xRot;
    yRotOut = yRot;
    zRotOut = zRot;
}

void MotionEvent::setRotations(const int &xRotIn, const int &yRotIn, const int &zRotIn)
{
    int motionDataArray[6] = {xTrans, yTrans, zTrans, xRotIn, yRotIn, zRotIn};
    importSettings(motionDataArray);
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
