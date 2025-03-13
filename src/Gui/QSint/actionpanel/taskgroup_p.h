/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef TASKGROUP_P_H
#define TASKGROUP_P_H

#include "actionlabel.h"
#include "actionpanelscheme.h"

#include <QBoxLayout>
#include <QFrame>


namespace QSint
{


class TaskGroup : public QFrame
{
  using BaseClass = QFrame;

public:
  TaskGroup(QWidget *parent, bool hasHeader = false);

  inline QBoxLayout* groupLayout()
  {
    return (QBoxLayout*)layout();
  }

  bool addActionLabel(ActionLabel *label, bool addToLayout, bool addStretch);

  bool addWidget(QWidget *widget, bool addToLayout, bool addStretch);

  QPixmap transparentRender();

protected:
  void keyPressEvent ( QKeyEvent * event ) override;
  void keyReleaseEvent ( QKeyEvent * event ) override;

  ActionPanelScheme *myScheme;

  bool myHasHeader;
};


}

#endif // TASKGROUP_P_H
