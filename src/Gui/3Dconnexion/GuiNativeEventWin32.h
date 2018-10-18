#ifndef GUINATIVEEVENT_H
#define GUINATIVEEVENT_H

#include <QObject>

#include "3Dconnexion/MouseParameters.h"

#include <vector>
#include <map>

//#define _WIN32_WINNT 0x0501  //target at least windows XP
#include <Windows.h>
#include <QAbstractNativeEventFilter>


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
		GUIApplicationNativeEventAware *mainApp;

    class RawInputEventFilter : public QAbstractNativeEventFilter
    {
    public:
        typedef bool (*EventFilter)(void *message, long *result);
        RawInputEventFilter(EventFilter filter) : eventFilter(filter) {
        }
        virtual ~RawInputEventFilter() {
        }

        virtual bool nativeEventFilter(const QByteArray & /*eventType*/, void *message, long *result) {
            return eventFilter(message, result);
        }

    private:
        EventFilter eventFilter;
    };

    public:
        static bool Is3dmouseAttached();

        I3dMouseParam& MouseParams();
        const I3dMouseParam& MouseParams() const;

        virtual void Move3d(HANDLE device, std::vector<float>& motionData);
        virtual void On3dmouseKeyDown(HANDLE device, int virtualKeyCode);
        virtual void On3dmouseKeyUp(HANDLE device, int virtualKeyCode);

    private:
        bool InitializeRawInput(HWND hwndTarget);
        static bool RawInputEventFilter(void* msg, long* result);
        void OnRawInput(UINT nInputCode, HRAWINPUT hRawInput);
        UINT GetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader);
        bool TranslateRawInputData(UINT nInputCode, PRAWINPUT pRawInput);
        bool ParseRawInput(UINT nInputCode, PRAWINPUT pRawInput);
        void On3dmouseInput();

        class TInputData
        {
        public:
            TInputData() : fAxes(6) {}

            bool IsZero() {
                return (0.==fAxes[0] && 0.==fAxes[1] && 0.==fAxes[2] &&
                        0.==fAxes[3] && 0.==fAxes[4] && 0.==fAxes[5]);
            }

            int fTimeToLive; // For telling if the device was unplugged while sending data
            bool fIsDirty;
            std::vector<float>     fAxes;
        };

        HWND fWindow;

        // Data cache to handle multiple rawinput devices
        std::map< HANDLE, TInputData>       fDevice2Data;
        std::map< HANDLE, unsigned long>    fDevice2Keystate;
        // 3dmouse parameters
        MouseParameters f3dMouseParams;     // Rotate, Pan Zoom etc.
        // use to calculate distance traveled since last event
        DWORD fLast3dmouseInputTime;
        static Gui::GUIApplicationNativeEventAware* gMouseInput;
	};
}

#endif //GUINATIVEEVENT_H

