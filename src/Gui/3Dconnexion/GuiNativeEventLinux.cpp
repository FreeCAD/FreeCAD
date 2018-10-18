 /*
Implementation by Torsten Sadowski 2018
 */

#include "GuiNativeEventLinux.h"

#include "GuiApplicationNativeEventAware.h"
#include "SpaceballEvent.h"
#include <FCConfig.h>
#include <Base/Console.h>
#include <QMainWindow>
#include <QTimer>

#include <spnav.h>

Gui::GuiNativeEvent::GuiNativeEvent(Gui::GUIApplicationNativeEventAware *app)
: QObject(app)
{
	mainApp = app;
}

Gui::GuiNativeEvent::~GuiNativeEvent()
{
    if (spnav_close())
        Base::Console().Log("Couldn't disconnect from spacenav daemon\n");
    else
        Base::Console().Log("Disconnected from spacenav daemon\n");
}

void Gui::GuiNativeEvent::initSpaceball(QMainWindow *window)
{
	Q_UNUSED(window)
    if (spnav_open() == -1) {
        Base::Console().Log("Couldn't connect to spacenav daemon\n");
    } else {
        Base::Console().Log("Connected to spacenav daemon\n");
        QTimer* SpacenavPollTimer = new QTimer(this);
		connect(SpacenavPollTimer, &QTimer::timeout, this, &GuiNativeEvent::pollSpacenav);
		SpacenavPollTimer->start(20);
		mainApp->setSpaceballPresent(true);
    }
}

void Gui::GuiNativeEvent::pollSpacenav()
{
	spnav_event ev;
	while(spnav_poll_event(&ev))
	{
		QWidget *currentWidget = mainApp->focusWidget();
		if (!currentWidget)
			return;
		//if (!setOSIndependentMotionData()) return;
		//importSettings();
		switch (ev.type)
		{
			case SPNAV_EVENT_MOTION:
			{
				Spaceball::MotionEvent *motionEvent = new Spaceball::MotionEvent();
				motionEvent->setTranslations(ev.motion.x, ev.motion.y, ev.motion.z);
				motionEvent->setRotations(ev.motion.rx, ev.motion.ry, ev.motion.rz);
				mainApp->postEvent(currentWidget, motionEvent);
				break;
			}
			case SPNAV_EVENT_BUTTON:
			{
				Spaceball::ButtonEvent *buttonEvent = new Spaceball::ButtonEvent();
				buttonEvent->setButtonNumber(ev.button.bnum);
				if (ev.button.press)
				{
					buttonEvent->setButtonStatus(Spaceball::BUTTON_PRESSED);
				}
				else
				{
					buttonEvent->setButtonStatus(Spaceball::BUTTON_RELEASED);
				}
				mainApp->postEvent(currentWidget, buttonEvent);
				break;
			}
		}
	}
}
