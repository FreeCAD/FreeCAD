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
  m_opacity(0.1),
  myButton(nullptr)
{
    setProperty("class", "header");

    myTitle = new ActionLabel(this);
    myTitle->setProperty("class", "header");
    myTitle->setText(title);
    myTitle->setIcon(icon);
    myTitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    connect(myTitle, &ActionLabel::clicked, this, &TaskHeader::fold);

    QHBoxLayout *hbl = new QHBoxLayout();
    hbl->setContentsMargins(2, 2, 2, 2);
    setLayout(hbl);

    hbl->addWidget(myTitle);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    setScheme(ActionPanelScheme::defaultScheme());
    //myTitle->setSchemePointer(&myLabelScheme);

    setExpandable(myExpandable);
}

void TaskHeader::setExpandable(bool expandable)
{
    if (expandable) {
        myExpandable = true;

        if (myButton)
            return;

        myButton = new QLabel(this);
        myButton->installEventFilter(this);
        myButton->setFixedSize(myScheme->headerButtonSize);
        layout()->addWidget(myButton);
        changeIcons();
        myButton->setProperty("fold", m_fold);

    } else {
        myExpandable = false;

        if (!myButton)
            return;

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
      if (myExpandable)
        fold();
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
    //myLabelScheme = &(scheme->headerLabelScheme);
    setStyleSheet(myScheme->actionStyle);

    if (myExpandable) {
      //setCursor(myLabelScheme->cursorOver ? Qt::PointingHandCursor : cursor());
      changeIcons();
    }

    setFixedHeight(scheme->headerSize);

    update();
  }
}

void TaskHeader::paintEvent ( QPaintEvent * event )
{
  QPainter p(this);

  if (myScheme->headerAnimation)
    p.setOpacity(m_opacity+0.7);

//  p.setPen(m_over ? myScheme->headerBorderOver : myScheme->headerBorder);
//  p.setBrush(m_over ? myScheme->headerBackgroundOver : myScheme->headerBackground);

//  myScheme->headerCorners.draw(&p, rect());

  BaseClass::paintEvent(event);
}

void TaskHeader::animate()
{
  if (!myScheme->headerAnimation)
    return;

  if (!isEnabled()) {
    m_opacity = 0.1;
    update();
    return;
  }

  if (m_over) {
    if (m_opacity >= 0.3) {
      m_opacity = 0.3;
      return;
    }
    m_opacity += 0.05;
  } else {
    if (m_opacity <= 0.1) {
      m_opacity = 0.1;
      return;
    }
    m_opacity = qMax(0.1, m_opacity-0.05);
  }

  QTimer::singleShot(100, this, &TaskHeader::animate);
  update();
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
void TaskHeader::enterEvent ( QEvent * /*event*/ )
#else
void TaskHeader::enterEvent ( QEnterEvent * /*event*/ )
#endif
{
  m_over = true;

  if (isEnabled())
    QTimer::singleShot(100, this, &TaskHeader::animate);

  update();
}

void TaskHeader::leaveEvent ( QEvent * /*event*/ )
{
  m_over = false;

  if (isEnabled())
    QTimer::singleShot(100, this, &TaskHeader::animate);

  update();
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
  if (!myButton)
    return;

  if (m_buttonOver)
  {
    if (m_fold)
      myButton->setPixmap(myScheme->headerButtonFoldOver);
    else
      myButton->setPixmap(myScheme->headerButtonUnfoldOver);
  } else
  {
    if (m_fold)
      myButton->setPixmap(myScheme->headerButtonFold);
    else
      myButton->setPixmap(myScheme->headerButtonUnfold);
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
