#ifndef GUINATIVEEVENT_H
#define GUINATIVEEVENT_H

#include <QObject>

#if QT_VERSION >= 0x050000
  #include <QAbstractNativeEventFilter>
  #include <xcb/xcb.h>
  #include <xcb/xproto.h>
#endif

class QMainWindow;
class GUIApplicationNativeEventAware;

namespace Gui
{
	class GUIApplicationNativeEventAware;

	class GuiNativeEvent : public QObject
	{
#include "GuiNativeEventCommon.h"
    public:
  #if QT_VERSION >= 0x050000
        static bool xcbEventFilter(void *message, long* result);
  #else
        bool x11EventFilter(XEvent *event);
  #endif // if/else QT_VERSION >= 0x050000
	};
}

#endif //GUINATIVEEVENT_H

