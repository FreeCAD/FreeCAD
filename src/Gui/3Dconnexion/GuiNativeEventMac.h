#ifndef GUINATIVEEVENT_H
#define GUINATIVEEVENT_H

#include <vector>
#include <QObject>

class QMainWindow;
class GUIApplicationNativeEventAware;

#include <IOKit/IOKitLib.h>
#include <ConnexionClientAPI.h>
// Note that InstallConnexionHandlers will be replaced with
// SetConnexionHandlers "in the future".
extern OSErr InstallConnexionHandlers(ConnexionMessageHandlerProc messageHandler,
                                      ConnexionAddedHandlerProc addedHandler,
                                      ConnexionRemovedHandlerProc removedHandler)
                                      __attribute__((weak_import));
extern UInt16 RegisterConnexionClient(UInt32 signature, UInt8 *name, UInt16 mode,
                                      UInt32 mask) __attribute__((weak_import));
extern void UnregisterConnexionClient(UInt16 clientID) __attribute__((weak_import));
extern void CleanupConnexionHandlers(void) __attribute__((weak_import));

namespace Gui
{
	class GUIApplicationNativeEventAware;

	class GuiNativeEvent : public QObject
	{
	#include "GuiNativeEventCommon.h"
	private:
        static UInt16 tdxClientID; /* ID assigned by the driver */
        static uint32_t lastButtons;
        static void tdx_drv_handler( io_connect_t connection,
                                     natural_t messageType,
                                     void *messageArgument );
	};
}

#endif //GUINATIVEEVENT_H

