#ifndef TASKGROUP_P_H
#define TASKGROUP_P_H

#include "actionpanelscheme.h"
#include "actionlabel.h"

#include <QFrame>
#include <QBoxLayout>


namespace QSint
{


class TaskGroup : public QFrame
{
  typedef QFrame BaseClass;

public:
  TaskGroup(QWidget *parent, bool hasHeader = false);

  void setScheme(ActionPanelScheme *scheme);

  inline QBoxLayout* groupLayout()
  {
    return (QBoxLayout*)layout();
  }

  bool addActionLabel(ActionLabel *label, bool addToLayout, bool addStretch);

  bool addWidget(QWidget *widget, bool addToLayout, bool addStretch);

  QPixmap transparentRender();

protected:
  virtual void paintEvent ( QPaintEvent * event );
  virtual void keyPressEvent ( QKeyEvent * event );
  virtual void keyReleaseEvent ( QKeyEvent * event );

  ActionPanelScheme *myScheme;

  bool myHasHeader;
};


}

#endif // TASKGROUP_P_H
