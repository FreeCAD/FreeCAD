#ifndef GUINATIVEEVENT_H
#define GUINATIVEEVENT_H

#include <vector>
#include <QObject>

class QMainWindow;

namespace Gui
{
	class GUIApplicationNativeEventAware;

	class GuiNativeEvent : public QObject
	{
#include "GuiNativeEventCommon.h"
	private slots:
		void pollSpacenav();
	};
}

#endif //GUINATIVEEVENT_H

