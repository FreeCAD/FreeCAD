/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef TASKHEADER_P_H
#define TASKHEADER_P_H

#include "actionlabel.h"
#include "actionpanelscheme.h"

#include <QLabel>


namespace QSint
{


class TaskHeader : public QFrame
{
  Q_OBJECT

  typedef QFrame BaseClass;

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

protected Q_SLOTS:
  void animate();

protected:
  void paintEvent ( QPaintEvent * event ) override;
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
  void enterEvent ( QEvent * event ) override;
#else
  void enterEvent ( QEnterEvent * event ) override;
#endif
  void leaveEvent ( QEvent * event ) override;
  void mouseReleaseEvent ( QMouseEvent * event ) override;
  void keyPressEvent ( QKeyEvent * event ) override;
  void keyReleaseEvent ( QKeyEvent * event ) override;

  bool eventFilter(QObject *obj, QEvent *event) override;

  void changeIcons();

  ActionPanelScheme *myScheme;

  bool myExpandable;
  bool m_over, m_buttonOver, m_fold;
  double m_opacity;

  ActionLabel *myTitle;
  QLabel *myButton;
};


}

#endif // TASKHEADER_P_H
