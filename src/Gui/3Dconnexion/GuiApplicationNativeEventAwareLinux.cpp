 /*
Implementation by Torsten Sadowski 2018
 */

#include "GuiApplicationNativeEventAware.h"
#include "SpaceballEvent.h"
#include <QWidget>

#if defined(SPNAV_FOUND)
  #include <spnav.h>
#endif

void Gui::GUIApplicationNativeEventAware::pollSpacenav()
{
	spnav_event ev;
	while(spnav_poll_event(&ev))
	{
		QWidget *currentWidget = this->focusWidget();
		if (!currentWidget)
			return;
		if (!setOSIndependentMotionData()) return;
		importSettings();
		switch (ev.type)
		{
			case SPNAV_EVENT_MOTION:
			{
				Spaceball::MotionEvent *motionEvent = new Spaceball::MotionEvent();
				motionEvent->setTranslations(ev.motion.x, ev.motion.y, ev.motion.z);
				motionEvent->setRotations(ev.motion.rx, ev.motion.ry, ev.motion.rz);
				this->postEvent(currentWidget, motionEvent);
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
				break;
			}
		}
	}
}
