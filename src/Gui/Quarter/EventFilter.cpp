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

/*!  \class SIM::Coin3D::Quarter::EventFilter EventFilter.h Quarter/eventhandlers/EventFilter.h

*/

#include <QEvent>
#include <QMouseEvent>

#include "QuarterWidget.h"
#include "devices/Keyboard.h"
#include "devices/Mouse.h"
#include "devices/SpaceNavigatorDevice.h"
#include "eventhandlers/EventFilter.h"


namespace SIM { namespace Coin3D { namespace Quarter {

class EventFilterP {
public:
  QList<InputDevice *> devices;
  QuarterWidget * quarterwidget;
  QPoint globalmousepos;
  SbVec2s windowsize;

  void trackWindowSize(QResizeEvent * event)
  {
    this->windowsize = SbVec2s(event->size().width(),
                               event->size().height());

    Q_FOREACH(InputDevice * device, this->devices) {
      device->setWindowSize(this->windowsize);
    }
  }

  void trackPointerPosition(QMouseEvent * event)
  {
    assert(this->windowsize[1] != -1);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    this->globalmousepos = event->globalPos();
#else
    this->globalmousepos = event->globalPosition().toPoint();
#endif

    SbVec2s mousepos(event->pos().x(), this->windowsize[1] - event->pos().y() - 1);
    // the following corrects for high-dpi displays (e.g. mac retina)
    mousepos *= quarterwidget->devicePixelRatio();
    Q_FOREACH(InputDevice * device, this->devices) {
      device->setMousePosition(mousepos);
    }
  }
};

#define PRIVATE(obj) obj->pimpl

}}} // namespace

using namespace SIM::Coin3D::Quarter;

EventFilter::EventFilter(QObject * parent)
  : QObject(parent)
{
  PRIVATE(this) = new EventFilterP;

  QuarterWidget* quarter = dynamic_cast<QuarterWidget *>(parent);
  PRIVATE(this)->quarterwidget = quarter;
  assert(PRIVATE(this)->quarterwidget);

  PRIVATE(this)->windowsize = SbVec2s(PRIVATE(this)->quarterwidget->width(),
                                      PRIVATE(this)->quarterwidget->height());

  PRIVATE(this)->devices += new Mouse(quarter);
  PRIVATE(this)->devices += new Keyboard(quarter);

#ifdef HAVE_SPACENAV_LIB
  PRIVATE(this)->devices += new SpaceNavigatorDevice(quarter);
#endif // HAVE_SPACENAV_LIB

}

EventFilter::~EventFilter()
{
  qDeleteAll(PRIVATE(this)->devices);
  delete PRIVATE(this);
}

/*!
  Adds a device for event translation
 */
void 
EventFilter::registerInputDevice(InputDevice * device)
{
  PRIVATE(this)->devices += device;
}

/*!
  Removes a device from event translation
 */
void 
EventFilter::unregisterInputDevice(InputDevice * device)
{
  int i = PRIVATE(this)->devices.indexOf(device);
  if (i != -1) {
    PRIVATE(this)->devices.removeAt(i);
  }
}

/*! Translates Qt Events into Coin events and passes them on to the
  event QuarterWidget for processing. If the event can not be
  translated or processed, it is forwarded to Qt and the method
  returns false.
 */
bool
EventFilter::eventFilter(QObject * obj, QEvent * qevent)
{
  Q_UNUSED(obj); 
  // make sure every device has updated screen size and mouse position
  // before translating events
  switch (qevent->type()) {
  case QEvent::MouseMove:
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease:
  case QEvent::MouseButtonDblClick:
    PRIVATE(this)->trackPointerPosition(static_cast<QMouseEvent *>(qevent));
    break;
  case QEvent::Resize:
    PRIVATE(this)->trackWindowSize(static_cast<QResizeEvent *>(qevent));
    break;
  default:
    break;
  }

  // translate QEvent into SoEvent and see if it is handled by scene
  // graph
  Q_FOREACH(InputDevice * device, PRIVATE(this)->devices) {
    const SoEvent * soevent = device->translateEvent(qevent);
    if (soevent && PRIVATE(this)->quarterwidget->processSoEvent(soevent)) {
      return true;
    }
  }
  return false;
}

/*!
  Returns mouse position in global coordinates
 */
const QPoint &
EventFilter::globalMousePosition() const
{
  return PRIVATE(this)->globalmousepos;
}

#undef PRIVATE
