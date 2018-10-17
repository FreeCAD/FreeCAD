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

#include <QGlobalStatic>
#include <QMainWindow>
#include <QWidget>
#include <FCConfig.h>
#include <Base/Console.h>
#include "GuiApplicationNativeEventAware.h"
#include "SpaceballEvent.h"
#include "Application.h"
#if defined(Q_OS_LINUX) && defined(SPNAV_FOUND)
  #include <spnav.h>

  #if QT_VERSION >= 0x050000
    #include <QTimer>
    #undef Bool
    #undef CursorShape
    #undef Expose
    #undef KeyPress
    #undef KeyRelease
    #undef FocusIn
    #undef FocusOut
    #undef FontChange
    #undef None
    #undef Status
    #undef Unsorted
    #undef False
    #undef True
    #undef Complex
  #endif // #if QT_VERSION >= 0x050000

#endif // if defined(Q_OS_LINUX) && defined(SPNAV_FOUND)

#ifdef _USE_3DCONNEXION_SDK
//windows
#ifdef Q_OS_WIN
Gui::GUIApplicationNativeEventAware* Gui::GUIApplicationNativeEventAware::gMouseInput = 0;
#endif
#endif //_USE_3DCONNEXION_SDK

Gui::GUIApplicationNativeEventAware::GUIApplicationNativeEventAware(int &argc, char *argv[]) :
        QApplication (argc, argv), spaceballPresent(false)
{
    mainWindow = 0;
}

Gui::GUIApplicationNativeEventAware::~GUIApplicationNativeEventAware()
{
#if defined(Q_OS_LINUX) && defined(SPNAV_FOUND)
    if (spnav_close())
        Base::Console().Log("Couldn't disconnect from spacenav daemon\n");
    else
        Base::Console().Log("Disconnected from spacenav daemon\n");

#elif defined(Q_OS_WIN) && defined(_USE_3DCONNEXION_SDK)
    if (gMouseInput == this) {
        gMouseInput = 0;
        Base::Console().Log("3Dconnexion device detached.\n");
    }

#elif defined(Q_OS_MACX) && defined(_USE_3DCONNEXION_SDK)
    // if 3Dconnexion library was loaded at runtime
    if (InstallConnexionHandlers) {
        // Close our connection with the 3dx driver
        if (tdxClientID)
            UnregisterConnexionClient(tdxClientID);
        CleanupConnexionHandlers();
        Base::Console().Log("Disconnected from 3Dconnexion driver\n");
    }
#endif // Platform switch
}

void Gui::GUIApplicationNativeEventAware::initSpaceball(QMainWindow *window)
{
    mainWindow = window;

#if defined(Q_OS_LINUX) && defined(SPNAV_FOUND)
    if (spnav_open() == -1) {
        Base::Console().Log("Couldn't connect to spacenav daemon\n");
    } else {
        Base::Console().Log("Connected to spacenav daemon\n");
        spaceballPresent = true;

    #if QT_VERSION >= 0x050000
        QTimer* SpacenavPollTimer = new QTimer(this);
		connect(SpacenavPollTimer, &QTimer::timeout, this, &GUIApplicationNativeEventAware::pollSpacenav);
		SpacenavPollTimer->start(20);
    #endif // #if QT_VERSION >= 0x050000
    }

#elif defined(Q_OS_WIN) && defined(_USE_3DCONNEXION_SDK)
    spaceballPresent = Is3dmouseAttached();

    if (spaceballPresent) {
        fLast3dmouseInputTime = 0;

        if (InitializeRawInput((HWND)mainWindow->winId())){
            gMouseInput = this;
#if QT_VERSION >= 0x050000
            qApp->installNativeEventFilter(new Gui::RawInputEventFilter(Gui::GUIApplicationNativeEventAware::RawInputEventFilter));
#else
            qApp->setEventFilter(Gui::GUIApplicationNativeEventAware::RawInputEventFilter);
#endif
            Base::Console().Log("3Dconnexion device initialized.\n");
        } else {
            Base::Console().Log("3Dconnexion device is attached, but not initialized.\n");
        }
    } else {
        Base::Console().Log("3Dconnexion device not attached.\n");
    }

#elif defined(Q_OS_MACX) && defined(_USE_3DCONNEXION_SDK)
    OSStatus err;
    /* make sure the framework is installed */
    if (InstallConnexionHandlers == NULL)
    {
        Base::Console().Log("3Dconnexion framework not found!\n");
        return;
    }
    /* install 3dx message handler in order to receive driver events */
    err = InstallConnexionHandlers(tdx_drv_handler, 0L, 0L);
    assert(err == 0);
    if (err)
    {
        Base::Console().Log("Error installing 3Dconnexion handler\n");
        return;
    }
    /* register our app with the driver */
    // Pascal string Application name required to register driver for application
    UInt8  tdxAppName[] = {7,'F','r','e','e','C','A','D'};
    // 32bit appID to register driver for application
    UInt32 tdxAppID = 'FCAd';
    tdxClientID = RegisterConnexionClient( tdxAppID, tdxAppName,
                                           kConnexionClientModeTakeOver,
                                           kConnexionMaskAll );
    if (tdxClientID == 0)
    {
        Base::Console().Log("Couldn't connect to 3Dconnexion driver\n");
        return;
    }
    
    Base::Console().Log("3Dconnexion driver initialized. Client ID: %d\n", tdxClientID);
    spaceballPresent = true;
#endif // Platform switch

    Spaceball::MotionEvent::MotionEventType = QEvent::registerEventType();
    Spaceball::ButtonEvent::ButtonEventType = QEvent::registerEventType();
}

bool Gui::GUIApplicationNativeEventAware::processSpaceballEvent(QObject *object, QEvent *event)
{
	std::cout << "Gui::GUIApplicationNativeEventAware::processSpaceballEvent" << std::endl;
    if (!activeWindow()) {
        qDebug("No active window\n");
        return true;
    }

    QApplication::notify(object, event);
    if (event->type() == Spaceball::MotionEvent::MotionEventType)
    {
        Spaceball::MotionEvent *motionEvent = dynamic_cast<Spaceball::MotionEvent*>(event);
        if (!motionEvent)
            return true;
        if (!motionEvent->isHandled())
        {
            //make a new event and post to parent.
            Spaceball::MotionEvent *newEvent = new Spaceball::MotionEvent(*motionEvent);
            postEvent(object->parent(), newEvent);
        }
    }

    if (event->type() == Spaceball::ButtonEvent::ButtonEventType)
    {
        Spaceball::ButtonEvent *buttonEvent = dynamic_cast<Spaceball::ButtonEvent*>(event);
        if (!buttonEvent)
            return true;
        if (!buttonEvent->isHandled())
        {
            //make a new event and post to parent.
            Spaceball::ButtonEvent *newEvent = new Spaceball::ButtonEvent(*buttonEvent);
            postEvent(object->parent(), newEvent);
        }
    }
    return true;
}


float Gui::GUIApplicationNativeEventAware::convertPrefToSensitivity(int value)
{
    if (value < 0)
    {
        return ((0.9/50)*float(value) + 1);
    }
    else
    {
        return ((2.5/50)*float(value) + 1);
    }
}

// This function modifies motionDataArray to be OS independent
// on some OSes these axes are inverted, and some are switched - this method sets them up like this:

// motionDataArray[0] - pan Left - Right with mouse - pan Left(Left) - Right(Left) on screen
// motionDataArray[1] - pan Front - Back with mouse - pan Up(Front) - Down(Back)   on screen
// motionDataArray[2] - pan Up - Down    with mouse - zoom In(Up) - Out(Down)      on screen
// motionDataArray[3] - lean mouse Left-Right       - rotate around Vertical    axis on screen
// motionDataArray[4] - lean mouse Front - Back     - rotate around Horizointal axis on screen on screen
// motionDataArray[5] - Spin mouse                  - rotate around "Zoom"      axis on screen


bool Gui::GUIApplicationNativeEventAware::setOSIndependentMotionData()
{
#ifdef SPNAV_FOUND
    int temp;
    motionDataArray[0] = -motionDataArray[0];
    motionDataArray[3] = -motionDataArray[3];

    temp = motionDataArray[1];
    motionDataArray[1] = -motionDataArray[2];
    motionDataArray[2] = -temp;

    temp = motionDataArray[4];
    motionDataArray[4] = -motionDataArray[5];
    motionDataArray[5] = -temp;
#elif defined(_USE_3DCONNEXION_SDK)
    motionDataArray[0] = -motionDataArray[0];
    motionDataArray[3] = -motionDataArray[3];
#else
    return false;
#endif
    return true;
}

void Gui::GUIApplicationNativeEventAware::importSettings()
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

    int i;

    if (flipXY) {
        bool  tempBool;
        float tempFloat;

        tempBool   = enabled[1];
        enabled[1] = enabled[2];
        enabled[2] = tempBool;

        tempBool   = enabled[4];
        enabled[4] = enabled[5];
        enabled[5] = tempBool;


        tempBool    = reversed[1];
        reversed[1] = reversed[2];
        reversed[2] = tempBool;

        tempBool    = reversed[4];
        reversed[4] = reversed[5];
        reversed[5] = tempBool;


        tempFloat      = sensitivity[1];
        sensitivity[1] = sensitivity[2];
        sensitivity[2] = tempFloat;

        tempFloat      = sensitivity[4];
        sensitivity[4] = sensitivity[5];
        sensitivity[5] = tempFloat;


        i = motionDataArray[1];
        motionDataArray[1] = motionDataArray[2];
        motionDataArray[2] = - i;

        i = motionDataArray[4];
        motionDataArray[4] = motionDataArray[5];
        motionDataArray[5] = - i;
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
            } else if (motionDataArray[i] == max) {
                flag = true;
            }
        }
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
}

#include "moc_GuiApplicationNativeEventAware.cpp"
