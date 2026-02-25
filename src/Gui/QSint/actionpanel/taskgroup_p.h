// SPDX-License-Identifier: LGPL-3.0-only
/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#pragma once

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
  void setScheme(ActionPanelScheme *scheme);

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
