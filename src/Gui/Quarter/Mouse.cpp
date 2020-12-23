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

/*!
  \class SIM::Coin3D::Quarter::Mouse Mouse.h Quarter/devices/Mouse.h

  \brief The Mouse class provides translation of mouse events on the
  QuarterWidget.
*/

#ifdef _MSC_VER
#pragma warning(disable : 4267)
#endif

#include <Quarter/devices/Mouse.h>

#include <QtCore/QEvent>
#include <QtCore/QSize>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#if QT_VERSION >= 0x050000
#include <QGuiApplication>
#endif

#include <Inventor/SbVec2s.h>
#include <Inventor/events/SoEvents.h>
#include <Inventor/errors/SoDebugError.h>
#if QT_VERSION >= 0x050000
#include <Quarter/QuarterWidget.h>
#endif

namespace SIM { namespace Coin3D { namespace Quarter {

class MouseP {
public:
  MouseP(Mouse * publ) {
    this->publ = publ;
    this->location2 = new SoLocation2Event;
    this->mousebutton = new SoMouseButtonEvent;
  }

  ~MouseP() {
    delete this->location2;
    delete this->mousebutton;
  }

  const SoEvent * mouseMoveEvent(QMouseEvent * event);
  const SoEvent * mouseWheelEvent(QWheelEvent * event);
  const SoEvent * mouseButtonEvent(QMouseEvent * event);

  void resizeEvent(QResizeEvent * event);

  class SoLocation2Event * location2;
  class SoMouseButtonEvent * mousebutton;
  SbVec2s windowsize;
  Mouse * publ;
};

}}} // namespace

using namespace SIM::Coin3D::Quarter;

#define PRIVATE(obj) obj->pimpl
#define PUBLIC(obj) obj->publ

Mouse::Mouse(void)
{
  PRIVATE(this) = new MouseP(this);
}

Mouse::Mouse(QuarterWidget *quarter) :
    InputDevice(quarter)
{
    PRIVATE(this) = new MouseP(this);
}

Mouse::~Mouse()
{
  delete PRIVATE(this);
}

/*! Translates from QMouseEvents to SoLocation2Events and
  SoMouseButtonEvents
 */
const SoEvent *
Mouse::translateEvent(QEvent * event)
{
  switch (event->type()) {
  case QEvent::MouseMove:
    return PRIVATE(this)->mouseMoveEvent((QMouseEvent *) event);
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease:
    // a dblclick event comes in a series of press, release, dblclick,
    // release, so we can simply treat it as an ordinary press event.
    // -mortene.
  case QEvent::MouseButtonDblClick:
    return PRIVATE(this)->mouseButtonEvent((QMouseEvent *) event);
  case QEvent::Wheel:
    return PRIVATE(this)->mouseWheelEvent((QWheelEvent *) event);
  case QEvent::Resize:
    PRIVATE(this)->resizeEvent((QResizeEvent *) event);
    return NULL;
  default:
    return NULL;
  }
}

void
MouseP::resizeEvent(QResizeEvent * event)
{
  this->windowsize = SbVec2s(event->size().width(),
                             event->size().height());
}

const SoEvent *
MouseP::mouseMoveEvent(QMouseEvent * event)
{
  PUBLIC(this)->setModifiers(this->location2, event);

  assert(this->windowsize[1] != -1);
  SbVec2s pos(event->pos().x(), this->windowsize[1] - event->pos().y() - 1);
  // the following corrects for high-dpi displays (e.g. mac retina)
#if QT_VERSION >= 0x050000
  pos *= publ->quarter->devicePixelRatio();
#endif
  this->location2->setPosition(pos);
  this->mousebutton->setPosition(pos);
  return this->location2;
}

const SoEvent *
MouseP::mouseWheelEvent(QWheelEvent * event)
{
  PUBLIC(this)->setModifiers(this->mousebutton, event);
  SbVec2s pos(event->pos().x(), PUBLIC(this)->windowsize[1] - event->pos().y() - 1);
  // the following corrects for high-dpi displays (e.g. mac retina)
#if QT_VERSION >= 0x050000
  pos *= publ->quarter->devicePixelRatio();
#endif
  this->location2->setPosition(pos);
  this->mousebutton->setPosition(pos);

  // QWheelEvent::delta() returns the distance that the wheel is
  // rotated, in eights of a degree. A positive value indicates that
  // the wheel was rotated forwards away from the user; a negative
  // value indicates that the wheel was rotated backwards toward the
  // user.
  (event->delta() > 0) ?
    this->mousebutton->setButton(SoMouseButtonEvent::BUTTON4) :
    this->mousebutton->setButton(SoMouseButtonEvent::BUTTON5);

  this->mousebutton->setState(SoButtonEvent::DOWN);
  return this->mousebutton;
}

const SoEvent *
MouseP::mouseButtonEvent(QMouseEvent * event)
{
  PUBLIC(this)->setModifiers(this->mousebutton, event);
  SbVec2s pos(event->pos().x(), PUBLIC(this)->windowsize[1] - event->pos().y() - 1);
  // the following corrects for high-dpi displays (e.g. mac retina)
#if QT_VERSION >= 0x050000
  pos *= publ->quarter->devicePixelRatio();
#endif
  this->location2->setPosition(pos);
  this->mousebutton->setPosition(pos);

  ((event->type() == QEvent::MouseButtonPress) ||
   (event->type() == QEvent::MouseButtonDblClick)) ?
    this->mousebutton->setState(SoButtonEvent::DOWN):
    this->mousebutton->setState(SoButtonEvent::UP);

  switch (event->button()) {
  case Qt::LeftButton:
    this->mousebutton->setButton(SoMouseButtonEvent::BUTTON1);
    break;
  case Qt::RightButton:
    this->mousebutton->setButton(SoMouseButtonEvent::BUTTON2);
    break;
  case Qt::MidButton:
    this->mousebutton->setButton(SoMouseButtonEvent::BUTTON3);
    break;
  default:
    this->mousebutton->setButton(SoMouseButtonEvent::ANY);
    SoDebugError::postInfo("Mouse::mouseButtonEvent",
                           "Unhandled ButtonState = %x", event->button());
    break;
  }
  return this->mousebutton;
}

#undef PRIVATE
#undef PUBLIC
