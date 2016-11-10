#include "InteractionMode.h"
#include <QtCore/QCoreApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QFocusEvent>
#include <Quarter/QuarterWidget.h>

/*
  Adjust how QuarterWidget reacts to alt key events
 */

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
InteractionMode::enabled(void) const
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
InteractionMode::on(void) const
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
