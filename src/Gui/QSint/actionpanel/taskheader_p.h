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

#include <QLabel>


namespace QSint
{


class TaskHeader : public QFrame
{
  Q_OBJECT

  using BaseClass = QFrame;

  friend class ActionGroup;

public:
  TaskHeader(const QIcon &icon, const QString &title, bool expandable, QWidget *parent = nullptr);

  inline bool expandable() const { return myExpandable; }
  void setExpandable(bool expandable);

  void setScheme(ActionPanelScheme *scheme);

Q_SIGNALS:
  void activated();

public:
  void setFold(bool);

public Q_SLOTS:
  void fold();

protected:
  void mouseReleaseEvent ( QMouseEvent * event ) override;
  void keyPressEvent ( QKeyEvent * event ) override;
  void keyReleaseEvent ( QKeyEvent * event ) override;

  bool eventFilter(QObject *obj, QEvent *event) override;

  void changeIcons();

  ActionPanelScheme *myScheme;

  bool myExpandable;
  bool m_over, m_buttonOver, m_fold;

  ActionLabel *myTitle;
  QLabel *myButton;
};


}
