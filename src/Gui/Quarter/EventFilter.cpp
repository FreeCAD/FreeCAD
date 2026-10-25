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

#include <memory>

#include <QEvent>
#include <QMouseEvent>
#include <QTimer>

#include "QuarterWidget.h"
#include "devices/Keyboard.h"
#include "devices/Mouse.h"
#include "devices/SpaceNavigatorDevice.h"
#include "eventhandlers/EventFilter.h"


namespace SIM { namespace Coin3D { namespace Quarter {

namespace {

QPointF getLocalPosition(const QMouseEvent* event)
{
  return event->position();
}

}

class EventFilterP {
public:
  QList<InputDevice *> devices;
  QuarterWidget * quarterwidget;
  QPoint globalmousepos;
  SbVec2s windowsize;

  std::unique_ptr<QMouseEvent> pendingmove;
  QTimer flushtimer;

  static bool mustFlushPendingMove(const QEvent * event)
  {
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::Leave:
    case QEvent::FocusOut:
    case QEvent::WindowDeactivate:
    case QEvent::Gesture:
    case QEvent::NativeGesture:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
      return true;
    default:
      return event->type() >= QEvent::User;
    }
  }

  bool deferMouseMove(const QMouseEvent * event)
  {
    if (!this->flushtimer.isActive()) {
      this->flushtimer.start();
      return false;
    }
    this->pendingmove.reset(event->clone());
    return true;
  }

  void discardPendingMove()
  {
    this->flushtimer.stop();
    this->pendingmove.reset();
  }

  void flushPendingMove()
  {
    this->flushtimer.stop();
    if (this->pendingmove) {
      std::unique_ptr<QMouseEvent> event = std::move(this->pendingmove);
      this->translateAndProcess(event.get());
    }
  }

  // translate QEvent into SoEvent and see if it is handled by scene graph
  bool translateAndProcess(QEvent * qevent)
  {
    for (InputDevice * device : std::as_const(this->devices)) {
      const SoEvent * soevent = device->translateEvent(qevent);
      if (soevent && this->quarterwidget->processSoEvent(soevent)) {
        return true;
      }
    }
    return false;
  }

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
    this->globalmousepos = event->globalPosition().toPoint();

    SbVec2s mousepos = InputDevice::toDevicePixelPosition(
        getLocalPosition(event),
        this->windowsize,
        quarterwidget->devicePixelRatio());
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

  PRIVATE(this)->flushtimer.setSingleShot(true);
  PRIVATE(this)->flushtimer.setInterval(0);
  connect(&PRIVATE(this)->flushtimer, &QTimer::timeout, this,
          [this] { PRIVATE(this)->flushPendingMove(); });

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
  event QuarterWidget for processing. If the event cannot be
  translated or processed, it is forwarded to Qt and the method
  returns false.

  Mouse move events arriving faster than they can be processed are
  coalesced. The first one is processed immediately, any following
  ones are swallowed and only the most recent of them is processed
  once the event loop has delivered all pending events, so that a
  burst of moves costs at most two scene graph traversals.
 */
bool
EventFilter::eventFilter(QObject * obj, QEvent * qevent)
{
  Q_UNUSED(obj); 
  // make sure every device has updated screen size and mouse position
  // before translating events
  switch (qevent->type()) {
  case QEvent::MouseMove: {
    auto mouseevent = dynamic_cast<QMouseEvent *>(qevent);
    PRIVATE(this)->trackPointerPosition(mouseevent);
    if (PRIVATE(this)->deferMouseMove(mouseevent)) {
      return true;
    }
    break;
  }
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease:
  case QEvent::MouseButtonDblClick:
    PRIVATE(this)->flushPendingMove();
    PRIVATE(this)->trackPointerPosition(dynamic_cast<QMouseEvent *>(qevent));
    break;
  case QEvent::Resize:
    PRIVATE(this)->flushPendingMove();
    PRIVATE(this)->trackWindowSize(dynamic_cast<QResizeEvent *>(qevent));
    break;
  case QEvent::Hide:
    PRIVATE(this)->discardPendingMove();
    break;
  default:
    if (EventFilterP::mustFlushPendingMove(qevent)) {
      PRIVATE(this)->flushPendingMove();
    }
    break;
  }

  return PRIVATE(this)->translateAndProcess(qevent);
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
