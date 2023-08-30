/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actiongroup.h"
#include "taskheader_p.h"
#include "taskgroup_p.h"

#include <QPainter>


namespace QSint
{


ActionGroup::ActionGroup(QWidget *parent)
  : QWidget(parent)
{
    myHeader = new TaskHeader(QPixmap(), "", false, this);
    myHeader->setVisible(false);
    init(false);
}

ActionGroup::ActionGroup(const QString &title, bool expandable, QWidget *parent)
  : QWidget(parent)
{
    myHeader = new TaskHeader(QPixmap(), title, expandable, this);
    init(true);
}

ActionGroup::ActionGroup(const QPixmap &icon, const QString &title, bool expandable, QWidget *parent)
  : QWidget(parent)
{
    myHeader = new TaskHeader(icon, title, expandable, this);
    init(true);
}

void ActionGroup::init(bool header)
{
  m_foldStep = 0;

  myScheme = ActionPanelScheme::defaultScheme();

  QVBoxLayout *vbl = new QVBoxLayout();
  vbl->setContentsMargins(0, 0, 0, 0);
  vbl->setSpacing(0);
  setLayout(vbl);

  vbl->addWidget(myHeader);

  myGroup = new TaskGroup(this, header);
  vbl->addWidget(myGroup);

  myDummy = new QWidget(this);
  vbl->addWidget(myDummy);
  myDummy->hide();

  connect(myHeader, &TaskHeader::activated, this, &ActionGroup::showHide);
}

void ActionGroup::setScheme(ActionPanelScheme *pointer)
{
  myScheme = pointer;
  myHeader->setScheme(pointer);
  myGroup->setScheme(pointer);
  update();
}

QBoxLayout* ActionGroup::groupLayout()
{
  return myGroup->groupLayout();
}

ActionLabel* ActionGroup::addAction(QAction *action, bool addToLayout, bool addStretch)
{
    if (!action)
        return nullptr;

    ActionLabel* label = new ActionLabel(action, this);
    myGroup->addActionLabel(label, addToLayout, addStretch);

    return label;
}

ActionLabel* ActionGroup::addActionLabel(ActionLabel *label, bool addToLayout, bool addStretch)
{
    if (!label)
        return nullptr;

    myGroup->addActionLabel(label, addToLayout, addStretch);

    return label;
}

bool ActionGroup::addWidget(QWidget *widget, bool addToLayout, bool addStretch)
{
    return myGroup->addWidget(widget, addToLayout, addStretch);
}

void ActionGroup::showHide()
{
  if (m_foldStep)
    return;

  if (!myHeader->expandable())
    return;

  if (myGroup->isVisible())
  {
    m_foldPixmap = myGroup->transparentRender();
//    m_foldPixmap = QPixmap::grabWidget(myGroup, myGroup->rect());

    m_tempHeight = m_fullHeight = myGroup->height();
    m_foldDelta = m_fullHeight / myScheme->groupFoldSteps;
    m_foldStep = myScheme->groupFoldSteps;
    m_foldDirection = -1;

    myGroup->hide();
    myDummy->setFixedSize(myGroup->size());
    myDummy->show();

    QTimer::singleShot(myScheme->groupFoldDelay, this, &ActionGroup::processHide);
  }
  else
  {
    m_foldStep = myScheme->groupFoldSteps;
    m_foldDirection = 1;
    m_tempHeight = 0;

    QTimer::singleShot(myScheme->groupFoldDelay, this, &ActionGroup::processShow);
  }

  myDummy->show();
}

void ActionGroup::processHide()
{
  if (!--m_foldStep) {
    myDummy->setFixedHeight(0);
    myDummy->hide();
    myHeader->setFold(false);
    setFixedHeight(myHeader->height());
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    return;
  }

  setUpdatesEnabled(false);

  m_tempHeight -= m_foldDelta;
  myDummy->setFixedHeight(m_tempHeight);
  setFixedHeight(myDummy->height()+myHeader->height());

  QTimer::singleShot(myScheme->groupFoldDelay, this, &ActionGroup::processHide);

  setUpdatesEnabled(true);
}

void ActionGroup::processShow()
{
  if (!--m_foldStep) {
    myDummy->hide();
    m_foldPixmap = QPixmap();
    myGroup->show();
    myHeader->setFold(true);
    setFixedHeight(m_fullHeight+myHeader->height());
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setMaximumHeight(QWIDGETSIZE_MAX);
    setMinimumHeight(0);
    return;
  }

  setUpdatesEnabled(false);

  m_tempHeight += m_foldDelta;
  myDummy->setFixedHeight(m_tempHeight);
  setFixedHeight(myDummy->height()+myHeader->height());

  QTimer::singleShot(myScheme->groupFoldDelay, this, &ActionGroup::processShow);

  setUpdatesEnabled(true);
}

void ActionGroup::paintEvent ( QPaintEvent * event )
{
  Q_UNUSED(event); 
  QPainter p(this);

  if (myDummy->isVisible()) {
    if (myScheme->groupFoldThaw) {
      if (m_foldDirection < 0)
        p.setOpacity((double)m_foldStep / myScheme->groupFoldSteps);
      else
        p.setOpacity((double)(myScheme->groupFoldSteps-m_foldStep) / myScheme->groupFoldSteps);
    }

    switch (myScheme->groupFoldEffect)
    {
      case ActionPanelScheme::ShrunkFolding:
        p.drawPixmap(myDummy->pos(), m_foldPixmap.scaled(myDummy->size()) );
        break;

      case ActionPanelScheme::SlideFolding:
        p.drawPixmap(myDummy->pos(), m_foldPixmap,
                     QRect(0, m_foldPixmap.height()-myDummy->height(),
                           m_foldPixmap.width(), myDummy->width()
                           )  );
        break;

      default:
        p.drawPixmap(myDummy->pos(), m_foldPixmap);
    }

    return;
  }
}


bool ActionGroup::isExpandable() const
{
    return myHeader->expandable();
}

void ActionGroup::setExpandable(bool expandable)
{
    myHeader->setExpandable(expandable);
}

bool ActionGroup::hasHeader() const
{
    return myHeader->isVisible();
}

void ActionGroup::setHeader(bool enable)
{
    myHeader->setVisible(enable);
}

QString ActionGroup::headerText() const
{
    return myHeader->myTitle->text();
}

void ActionGroup::setHeaderText(const QString & headerText)
{
    myHeader->myTitle->setText(headerText);
}


QSize ActionGroup::minimumSizeHint() const
{
    return {200,65};
}


}
