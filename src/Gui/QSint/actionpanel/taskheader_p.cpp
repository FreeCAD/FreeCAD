/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "taskheader_p.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QTimer>


namespace QSint
{

TaskHeader::TaskHeader(const QIcon &icon, const QString &title, bool expandable, QWidget *parent)
  : BaseClass(parent),
  myExpandable(expandable),
  m_over(false),
  m_buttonOver(false),
  m_fold(true),
  myButton(nullptr)
{
    setProperty("class", "header");

    myTitle = new ActionLabel(this);
    myTitle->setProperty("class", "header");
    myTitle->setText(title);
    myTitle->setIcon(icon);
    myTitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    QFont font = myTitle->font();
    font.setBold(true);
    myTitle->setFont(font);

    connect(myTitle, &ActionLabel::clicked, this, &TaskHeader::fold);

    QHBoxLayout *hbl = new QHBoxLayout();
    hbl->setContentsMargins(4, 2, 8, 2);
    setLayout(hbl);

    hbl->addWidget(myTitle);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    setScheme(ActionPanelScheme::defaultScheme());

    setExpandable(myExpandable);
}

void TaskHeader::setExpandable(bool expandable)
{
    if (expandable) {
        myExpandable = true;

        if (myButton) {
            return;
        }
        myButton = new QLabel(this);
        myButton->installEventFilter(this);
        myButton->setFixedSize(myScheme->headerButtonSize);
        layout()->addWidget(myButton);
        changeIcons();
        myButton->setProperty("fold", m_fold);

    } else {
        myExpandable = false;

        if (!myButton) {
            return;
        }
        myButton->removeEventFilter(this);
        myButton->setParent(nullptr);
        delete myButton;
        myButton = nullptr;
        changeIcons();
    }
}

bool TaskHeader::eventFilter(QObject *obj, QEvent *event)
{
  switch (event->type()) {
    case QEvent::MouseButtonPress:
      if (myExpandable) {
        fold();
      }
      return true;

    case QEvent::Enter:
      m_buttonOver = true;
      changeIcons();
      return true;

    case QEvent::Leave:
      m_buttonOver = false;
      changeIcons();
      return true;

    default:;
  }

  return BaseClass::eventFilter(obj, event);
}

void TaskHeader::setScheme(ActionPanelScheme *scheme)
{
  if (scheme) {
    myScheme = scheme;
    if (myExpandable) {
      changeIcons();
    }
    setFixedHeight(scheme->headerSize);
    update();
  }
}

void TaskHeader::fold()
{
  if (myExpandable) {
    Q_EMIT activated();
  }
}

void TaskHeader::setFold(bool on)
{
  if (myExpandable) {
    m_fold = on;
    changeIcons();
    if (myButton) {
      myButton->setProperty("fold", m_fold);
      if (myButton->style()) {
        myButton->style()->unpolish(myButton);
        myButton->style()->polish(myButton);
        myButton->update();
      }
    }
  }
}

void TaskHeader::changeIcons()
{
  if (!myButton) {
    return;
  }
  if (m_buttonOver)
  {
    if (m_fold) {
      myButton->setPixmap(myScheme->headerButtonFoldOver);
    }
    else {
      myButton->setPixmap(myScheme->headerButtonUnfoldOver);
    }
  } else
  {
    if (m_fold) {
      myButton->setPixmap(myScheme->headerButtonFold);
    }
    else {
      myButton->setPixmap(myScheme->headerButtonUnfold);
    }
  }

  myButton->setFixedSize(myScheme->headerButtonSize);
}

void TaskHeader::mouseReleaseEvent ( QMouseEvent * event )
{
  if (event->button() == Qt::LeftButton) {
    Q_EMIT activated();
  }
}

void TaskHeader::keyPressEvent ( QKeyEvent * event )
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

void TaskHeader::keyReleaseEvent ( QKeyEvent * event )
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
