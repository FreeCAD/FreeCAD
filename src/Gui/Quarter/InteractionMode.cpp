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

/*
  Adjust how QuarterWidget reacts to alt key events
 */

#include <QCoreApplication>
#include <QFocusEvent>
#include <QKeyEvent>

#include "InteractionMode.h"
#include "QuarterWidget.h"


using namespace SIM::Coin3D::Quarter;

InteractionMode::InteractionMode(QuarterWidget * quarterwidget)
  : QObject(quarterwidget)
{
  this->quarterwidget = quarterwidget;
  this->altkeydown = false;
  this->prevcursor = QCursor();
  this->prevnavstate =
    this->quarterwidget->getSoEventManager()->getNavigationState();

  this->isenabled = true;
}

InteractionMode::~InteractionMode()
{

}

void
InteractionMode::setEnabled(bool yes)
{
  this->isenabled = yes;
}

bool
InteractionMode::enabled() const
{
  return this->isenabled;
}

void
InteractionMode::setOn(bool on)
{
  if (!this->isenabled) {
    return;
  }

  SoEventManager * eventmanager = this->quarterwidget->getSoEventManager();

  if (on) {
    this->altkeydown = true;
    this->prevnavstate = eventmanager->getNavigationState();
    this->prevcursor = this->quarterwidget->cursor();
    this->quarterwidget->setCursor(this->quarterwidget->stateCursor("interact"));
    eventmanager->setNavigationState(SoEventManager::NO_NAVIGATION);
  } else {
    this->altkeydown = false;
    this->quarterwidget->setCursor(this->prevcursor);
    eventmanager->setNavigationState(this->prevnavstate);
  }
}

bool
InteractionMode::on() const
{
  return this->altkeydown;
}

bool
InteractionMode::eventFilter(QObject * obj, QEvent * event)
{
  if (!this->isenabled) {
    return false;
  }

  assert(obj == this->quarterwidget);

  switch (event->type()) {
  case QEvent::KeyPress:
    return this->keyPressEvent(dynamic_cast<QKeyEvent *>(event));
  case QEvent::KeyRelease:
    return this->keyReleaseEvent(dynamic_cast<QKeyEvent *>(event));
  case QEvent::FocusOut:
    return this->focusOutEvent(dynamic_cast<QFocusEvent *>(event));
  default:
    return QObject::eventFilter(obj, event);
  }
}

/*
  when alt is pressed, override navigation and allow scenegraph to
  process events so draggers and manipulators works
 */
bool
InteractionMode::keyPressEvent(QKeyEvent * event)
{
  if (!event ||
      !(event->key() == Qt::Key_Alt) ||
      !(event->modifiers() & Qt::AltModifier)) {
    return false;
  }

  this->setOn(true);
  return true;
}

bool
InteractionMode::keyReleaseEvent(QKeyEvent * event)
{
  if (!event || !(event->key() == Qt::Key_Alt)) {
    return false;
  }

  this->setOn(false);
  return true;
}

/*
  if we lose focus while alt is down, send an alt-release event
 */
bool
InteractionMode::focusOutEvent(QFocusEvent * event)
{
  Q_UNUSED(event); 
  if (this->altkeydown) {
    QKeyEvent keyevent(QEvent::KeyRelease, Qt::Key_Alt, Qt::NoModifier);
    return QCoreApplication::sendEvent(this->quarterwidget, &keyevent);
  }
  return false;
}
