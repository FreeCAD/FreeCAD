/*
Development tools and related technology provided under license from 3Dconnexion.
(c) 1992 - 2012 3Dconnexion. All rights reserved
*/

 /*
Implementation by Torsten Sadowski 2015
with special thanks to marcxs for making the first steps
 */

#include <unistd.h>
#include "GUIApplicationNativeEventAware.h"
#include "SpaceballEvent.h"
#include <QWidget>



UInt16 Gui::GUIApplicationNativeEventAware::tdxClientID = 0;
uint32_t Gui::GUIApplicationNativeEventAware::lastButtons = 0;

  /* ----------------------------------------------------------------------------
     Handler for driver events. This function is able to handle the events in
     different ways: (1) re-package the events as Carbon events, (2) compute
     them directly, (3) write the event info in a shared memory location for
     usage by reader threads.
  */
  void 
  Gui::GUIApplicationNativeEventAware::tdx_drv_handler(io_connect_t connection, 
		  natural_t messageType, 
		  void *messageArgument)
  {
    Q_UNUSED(connection)
    //printf("tdx_drv_handler\n");
    //printf("connection: %X\n", connection);
    //printf("messageType %c%c%c%c\n", messageType/0x1000000, messageType/0x10000, messageType/0x100, messageType);
    ConnexionDeviceStatePtr msg = (ConnexionDeviceStatePtr)messageArgument;
    
    switch(messageType)
      {
      case kConnexionMsgDeviceState:
	/* Device state messages are broadcast to all clients.  It is up to
	 * the client to figure out if the message is meant for them. This
	 * is done by comparing the "client" id sent in the message to our
	 * assigned id when the connection to the driver was established.
	 */
	//printf("msg->client: %d, tdxClientID: %d\n", msg->client, tdxClientID);	 
	if (msg->client == tdxClientID)
	  {
	    Gui::GUIApplicationNativeEventAware* qapp = static_cast<Gui::GUIApplicationNativeEventAware *>(Gui::GUIApplicationNativeEventAware::instance());
	    if (!qapp) return;
	    switch (msg->command)
	      {
	      case kConnexionCmdHandleAxis:
		{
		  //printf("TX: %d\n", msg->axis[0]);
		  //printf("TY: %d\n", msg->axis[1]);
		  //printf("TZ: %d\n", msg->axis[2]);
		  //printf("RX: %d\n", msg->axis[3]);
		  //printf("RY: %d\n", msg->axis[4]);
		  //printf("RZ: %d\n", msg->axis[5]);
		  qapp->motionDataArray[0] = msg->axis[0];                        
		  qapp->motionDataArray[1] = msg->axis[1];                        
		  qapp->motionDataArray[2] = msg->axis[2];                        
		  qapp->motionDataArray[3] = msg->axis[3];                        
		  qapp->motionDataArray[4] = msg->axis[4];                        
		  qapp->motionDataArray[5] = msg->axis[5];
		  qapp->Move3d();
		  break;
		}
                        
	      case kConnexionCmdHandleButtons:
		{
		  //printf("value: %d\n", msg->value);
		  //printf("buttons: %u\n", msg->buttons);
		  uint32_t changedButtons = msg->buttons ^ lastButtons;
		  uint32_t pressedButtons = msg->buttons & changedButtons;
		  uint32_t releasedButtons = lastButtons & changedButtons;
		  for (uint8_t bt = 0; bt < 32; bt++)
		    {
		      if (pressedButtons & 1)
			qapp->Button3d(true, bt);
		      pressedButtons = pressedButtons>>1;
		    }
		  for (uint8_t bt = 0; bt < 32; bt++)
		    {
		      if (releasedButtons & 1)
			qapp->Button3d(false, bt);
		      releasedButtons = releasedButtons>>1;
		    }
		  lastButtons = msg->buttons;
		  break;
		}
                        
	      default:
		break;

	      } /* switch */
	  }

	break;

      default:
	/* other messageTypes can happen and should be ignored */
	break;
      }
  }
  
Gui::GuiNativeEvent::GuiNativeEvent(Gui::GUIApplicationNativeEventAware *app)
: QObject(app)
{
	spaceballPresent = false;
	mainApp = app;
}

Gui::GuiNativeEvent::~GuiNativeEvent()
{
    // if 3Dconnexion library was loaded at runtime
    if (InstallConnexionHandlers) {
        // Close our connection with the 3dx driver
        if (tdxClientID)
            UnregisterConnexionClient(tdxClientID);
        CleanupConnexionHandlers();
        Base::Console().Log("Disconnected from 3Dconnexion driver\n");
    }
}

void Gui::GuiNativeEvent::initSpaceball(QMainWindow *window)
{
	Q_UNUSED(window)
    OSStatus err;
    /* make sure the framework is installed */
    if (InstallConnexionHandlers == NULL)
    {
        Base::Console().Log("3Dconnexion framework not found!\n");
        return;
    }
    /* install 3dx message handler in order to receive driver events */
    err = InstallConnexionHandlers(tdx_drv_handler, 0L, 0L);
    assert(err == 0);
    if (err)
    {
        Base::Console().Log("Error installing 3Dconnexion handler\n");
        return;
    }
    /* register our app with the driver */
    // Pascal string Application name required to register driver for application
    UInt8  tdxAppName[] = {7,'F','r','e','e','C','A','D'};
    // 32bit appID to register driver for application
    UInt32 tdxAppID = 'FCAd';
    tdxClientID = RegisterConnexionClient( tdxAppID, tdxAppName,
                                           kConnexionClientModeTakeOver,
                                           kConnexionMaskAll );
    if (tdxClientID == 0)
    {
        Base::Console().Log("Couldn't connect to 3Dconnexion driver\n");
        return;
    }
    
    Base::Console().Log("3Dconnexion driver initialized. Client ID: %d\n", tdxClientID);
    spaceballPresent = true;
}

/*!
	Called with the processed motion data when a 3D mouse event is received

	The default implementation emits a Move3d signal with the motion data
*/
void Gui::GUIApplicationNativeEventAware::Move3d()
{
  QWidget *currentWidget = this->focusWidget();
  if (!currentWidget)
    return;
    //currentWidget = mainWindow;

  if (!setOSIndependentMotionData()) return;
  importSettings();
  
  Spaceball::MotionEvent *motionEvent = new Spaceball::MotionEvent();
  
  motionEvent->setTranslations(motionDataArray[0], motionDataArray[1], motionDataArray[2]);
  motionEvent->setRotations(motionDataArray[3], motionDataArray[4], motionDataArray[5]);
  
  this->postEvent(currentWidget, motionEvent);
}

/*!
	Called when a 3D mouse key is pressed or released

	The default implementation emits a On3dmouseKeyDown signal with the key code.
*/
void Gui::GUIApplicationNativeEventAware::Button3d(bool buttonDown, int buttonNumber)
{
    QWidget *currentWidget = this->focusWidget();
    if (!currentWidget)
      return;
    // currentWidget = mainWindow;

    Spaceball::ButtonEvent *buttonEvent = new Spaceball::ButtonEvent();
    buttonEvent->setButtonNumber(buttonNumber);
    if (buttonDown)
      {
	//printf("Button %d pressed\n", buttonNumber);
	buttonEvent->setButtonStatus(Spaceball::BUTTON_PRESSED);
      }
    else
      {
	//printf("Button %d released\n", buttonNumber);
	buttonEvent->setButtonStatus(Spaceball::BUTTON_RELEASED);
      }
    this->postEvent(currentWidget, buttonEvent);
}

