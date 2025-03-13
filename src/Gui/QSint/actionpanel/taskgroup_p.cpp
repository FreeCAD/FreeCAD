/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "taskgroup_p.h"

#include <QApplication>
#include <QKeyEvent>


namespace QSint
{


TaskGroup::TaskGroup(QWidget *parent, bool hasHeader)
  : BaseClass(parent),
  myHasHeader(hasHeader)
{
    setProperty("class", "content");
    setProperty("header", hasHeader ? "true" : "false");

    QVBoxLayout *vbl = new QVBoxLayout();
    vbl->setContentsMargins(4, 4, 4, 4);
    vbl->setSpacing(0);
    setLayout(vbl);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
}

bool TaskGroup::addActionLabel(ActionLabel *label, bool addToLayout, bool addStretch)
{
  if (!label) {
      return false;
  }
  return addWidget(label, addToLayout, addStretch);
}

bool TaskGroup::addWidget(QWidget *widget, bool addToLayout, bool addStretch)
{
  if (!widget) {
      return false;
  }
  if (!addToLayout) {
      return true;
  }
  if (addStretch) {
    QHBoxLayout *hbl = new QHBoxLayout();
    hbl->setContentsMargins(0, 0, 0, 0);
    hbl->setSpacing(0);
    hbl->addWidget(widget);
    hbl->addStretch();

    groupLayout()->addLayout(hbl);
  }
  else {
      groupLayout()->addWidget(widget);
  }

  return true;
}

QPixmap TaskGroup::transparentRender()
{
  QPixmap pm(size());
  pm.fill(Qt::transparent);

  render(&pm, QPoint(0,0), rect(), DrawChildren | IgnoreMask);

  return pm;
}

void TaskGroup::keyPressEvent ( QKeyEvent * event )
{
  switch (event->key())
  {
    case Qt::Key_Down:
    {
      QKeyEvent ke(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
      QApplication::sendEvent(this, &ke);
      return;
    }

    case Qt::Key_Up:
    {
      QKeyEvent ke(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier);
      QApplication::sendEvent(this, &ke);
      return;
    }

    default:;
  }

  BaseClass::keyPressEvent(event);
}


void TaskGroup::keyReleaseEvent ( QKeyEvent * event )
{
  switch (event->key())
  {
    case Qt::Key_Down:
    {
      QKeyEvent ke(QEvent::KeyRelease, Qt::Key_Tab, Qt::NoModifier);
      QApplication::sendEvent(this, &ke);
      return;
    }

    case Qt::Key_Up:
    {
      QKeyEvent ke(QEvent::KeyRelease, Qt::Key_Tab, Qt::ShiftModifier);
      QApplication::sendEvent(this, &ke);
      return;
    }

    default:;
  }

  BaseClass::keyReleaseEvent(event);
}


}
