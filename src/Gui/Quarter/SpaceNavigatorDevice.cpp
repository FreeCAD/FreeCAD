/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#ifdef _MSC_VER
#pragma warning(disable : 4267)
#endif

#include <Quarter/devices/SpaceNavigatorDevice.h>

#include <QApplication>
#include <QWidget>
#include <QtCore/QEvent>

#include <Inventor/events/SoEvent.h>
#include <Inventor/events/SoMotion3Event.h>
#include <Inventor/events/SoSpaceballButtonEvent.h>

#include "NativeEvent.h"

#ifdef HAVE_SPACENAV_LIB
#include <spnav.h>
#endif //HAVE_SPACENAV_LIB

#include <cstdio>


namespace SIM { namespace Coin3D { namespace Quarter {
class SpaceNavigatorDeviceP {
public:
  SpaceNavigatorDeviceP(SpaceNavigatorDevice * master) {
    this->master = master;
    this->hasdevice = false;
    this->windowid = 0;
    this->motionevent = new SoMotion3Event;
    this->buttonevent = new SoSpaceballButtonEvent;
  }
  ~SpaceNavigatorDeviceP() {
    delete this->motionevent;
    delete this->buttonevent;
  }

  static bool customEventFilter(void * message, long * result);

  SpaceNavigatorDevice * master;
  bool hasdevice;
  WId windowid;

  SoMotion3Event * motionevent;
  SoSpaceballButtonEvent * buttonevent;

};
}}}


#define PRIVATE(obj) obj->pimpl
using namespace SIM::Coin3D::Quarter;

SpaceNavigatorDevice::SpaceNavigatorDevice()
{
  PRIVATE(this) = new SpaceNavigatorDeviceP(this);

#ifdef HAVE_SPACENAV_LIB
  PRIVATE(this)->hasdevice =
    spnav_open() == -1 ? false : true;

  // FIXME: Use a debugmessage mechanism instead? (20101020 handegar)
  if (!PRIVATE(this)->hasdevice) {
    fprintf(stderr, "Quarter:: Could not hook up to Spacenav device.\n");
  }

#endif // HAVE_SPACENAV_LIB
}

SpaceNavigatorDevice::SpaceNavigatorDevice(QuarterWidget *quarter) :
    InputDevice(quarter)
{
    PRIVATE(this) = new SpaceNavigatorDeviceP(this);

#ifdef HAVE_SPACENAV_LIB
    PRIVATE(this)->hasdevice =
            spnav_open() == -1 ? false : true;

    // FIXME: Use a debugmessage mechanism instead? (20101020 handegar)
    if (!PRIVATE(this)->hasdevice) {
        fprintf(stderr, "Quarter:: Could not hook up to Spacenav device.\n");
    }

#endif // HAVE_SPACENAV_LIB
}

SpaceNavigatorDevice::~SpaceNavigatorDevice()
{
  delete PRIVATE(this);
}


const SoEvent *
SpaceNavigatorDevice::translateEvent(QEvent * event)
{
  Q_UNUSED(event); 
  SoEvent * ret = NULL;

#ifdef HAVE_SPACENAV_LIB
  NativeEvent * ce = dynamic_cast<NativeEvent *>(event);
  if (ce && ce->getEvent()) {
    XEvent * xev = ce->getEvent();

    spnav_event spev;
    if(spnav_event(xev, &spev)) {
      if(spev.type == SPNAV_EVENT_MOTION) {
        // Add rotation
        const float axislen = sqrt(spev.motion.rx*spev.motion.rx +
                                   spev.motion.ry*spev.motion.ry +
                                   spev.motion.rz*spev.motion.rz);

	const float half_angle = axislen * 0.5 * 0.001;
	const float sin_half = sin(half_angle);
        SbRotation rot((spev.motion.rx / axislen) * sin_half,
                       (spev.motion.ry / axislen) * sin_half,
                       (spev.motion.rz / axislen) * sin_half,
                       cos(half_angle));
        PRIVATE(this)->motionevent->setRotation(rot);

        // Add translation
        SbVec3f pos(spev.motion.x * 0.001,
                    spev.motion.y * 0.001,
                    spev.motion.z * 0.001);
        PRIVATE(this)->motionevent->setTranslation(pos);

        ret = PRIVATE(this)->motionevent;
      }
      else if (spev.type == SPNAV_EVENT_BUTTON){
        if(spev.button.press) {
          PRIVATE(this)->buttonevent->setState(SoButtonEvent::DOWN);
          switch(spev.button.bnum) {
          case 0: PRIVATE(this)->buttonevent->setButton(SoSpaceballButtonEvent::BUTTON1);
            break;
          case 1: PRIVATE(this)->buttonevent->setButton(SoSpaceballButtonEvent::BUTTON2);
            break;
          case 2: PRIVATE(this)->buttonevent->setButton(SoSpaceballButtonEvent::BUTTON3);
            break;
          case 3: PRIVATE(this)->buttonevent->setButton(SoSpaceballButtonEvent::BUTTON4);
            break;
          case 4: PRIVATE(this)->buttonevent->setButton(SoSpaceballButtonEvent::BUTTON5);
            break;
          case 5: PRIVATE(this)->buttonevent->setButton(SoSpaceballButtonEvent::BUTTON6);
            break;
          case 6: PRIVATE(this)->buttonevent->setButton(SoSpaceballButtonEvent::BUTTON7);
            break;
          case 7: PRIVATE(this)->buttonevent->setButton(SoSpaceballButtonEvent::BUTTON8);
            break;
          default:
            // FIXME: Which button corresponds to the
            // SoSpaceballButtonEvent::PICK enum? (20101020 handegar)
            break;
          }
        }
        else {
          PRIVATE(this)->buttonevent->setState(SoButtonEvent::UP);
        }

        ret = PRIVATE(this)->buttonevent;
      }
      else {
        // Unknown Spacenav event.
        assert(0 && "Unknown event type");
      }
    }
  }
#endif // HAVE_SPACENAV_LIB

  return ret;
}


#undef PRIVATE
#undef PUBLIC
