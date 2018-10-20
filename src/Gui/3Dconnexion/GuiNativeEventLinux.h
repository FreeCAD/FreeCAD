#ifndef GUINATIVEEVENT_H
#define GUINATIVEEVENT_H

#include <QObject>

class QMainWindow;
class GUIApplicationNativeEventAware;

namespace Gui
{
	class GUIApplicationNativeEventAware;

	class GuiNativeEvent : public QObject
	{
#include "GuiNativeEventCommon.h"
	private:
		void pollSpacenav();
	};
}

#endif //GUINATIVEEVENT_H

