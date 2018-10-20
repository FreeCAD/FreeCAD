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
        bool xcbEventFilter(const xcb_client_message_event_t *message);
  #else
        bool x11EventFilter(XEvent *event);
  #endif // if/else QT_VERSION >= 0x050000
	};
}

#endif //GUINATIVEEVENT_H

