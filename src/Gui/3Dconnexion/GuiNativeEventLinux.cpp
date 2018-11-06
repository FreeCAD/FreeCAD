 /*
Implementation by Torsten Sadowski 2018
 */

#include "GuiNativeEventLinux.h"

#include "GuiApplicationNativeEventAware.h"
#include <FCConfig.h>
#include <Base/Console.h>
#include <QMainWindow>

#include <QSocketNotifier>

#include <spnav.h>

std::vector<int> Gui::GuiNativeEvent::motionDataArray(6,0);

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
		QSocketNotifier* SpacenavNotifier = new QSocketNotifier(spnav_fd(), QSocketNotifier::Read, this);
		connect(SpacenavNotifier, SIGNAL(activated(int)), this, SLOT(pollSpacenav())); 
		mainApp->setSpaceballPresent(true);
    }
}

void Gui::GuiNativeEvent::pollSpacenav()
{
	spnav_event ev;
	while(spnav_poll_event(&ev))
	{
		switch (ev.type)
		{
			case SPNAV_EVENT_MOTION:
			{
				motionDataArray[0] = -ev.motion.x;
				motionDataArray[1] = -ev.motion.z;
				motionDataArray[2] = -ev.motion.y;
				motionDataArray[3] = -ev.motion.rx;
				motionDataArray[4] = -ev.motion.rz;
				motionDataArray[5] = -ev.motion.ry;
				mainApp->postMotionEvent(motionDataArray);
				break;
			}
			case SPNAV_EVENT_BUTTON:
			{
				mainApp->postButtonEvent(ev.button.bnum, ev.button.press);
				break;
			}
		}
	}
}

#include "3Dconnexion/moc_GuiNativeEventLinux.cpp"
