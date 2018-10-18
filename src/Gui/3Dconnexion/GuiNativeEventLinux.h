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
	public:
		GuiNativeEvent(GUIApplicationNativeEventAware *app);
		~GuiNativeEvent();
		void initSpaceball(QMainWindow *window);
	private:
		GuiNativeEvent();
		GuiNativeEvent(GuiNativeEvent*);
		void pollSpacenav();
		GUIApplicationNativeEventAware *mainApp;
	};
}

#endif //GUINATIVEEVENT_H

