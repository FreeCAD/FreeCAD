/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef ACTIONGROUP_H
#define ACTIONGROUP_H

#include <QBoxLayout>
#include <QTimer>
#include <QWidget>
#include "qsint_global.h"


namespace QSint
{


class ActionLabel;
class ActionPanelScheme;


/**
    \brief Class representing a single group of actions similar to Windows XP task panels.
    \since 0.2

    \image html ActionGroup.png An example of ActionGroup

    ActionGroup consists from optional header and set of actions represented by ActionLabel.
    It can contain arbitrary widgets as well.
*/

class QSINT_EXPORT ActionGroup : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool expandable READ isExpandable WRITE setExpandable) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(bool header READ hasHeader WRITE setHeader) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QString headerText READ headerText WRITE setHeaderText) // clazy:exclude=qproperty-without-notify

public:
    /** Constructor. Creates ActionGroup without header.
      */
    explicit ActionGroup(QWidget *parent = nullptr);

    /** Constructor. Creates ActionGroup with header's
        text set to \a title, but with no icon.

        If \a expandable set to \a true (default), the group can be expanded/collapsed by the user.
      */
    explicit ActionGroup(const QString& title,
                         bool expandable = true,
                         QWidget *parent = nullptr);

    /** Constructor. Creates ActionGroup with header's
        text set to \a title and icon set to \a icon.

        If \a expandable set to \a true (default), the group can be expanded/collapsed by the user.
      */
    explicit ActionGroup(const QPixmap& icon,
                         const QString& title,
                         bool expandable = true,
                         QWidget *parent = nullptr);

    /** Creates action item from the \a action and returns it.

      If \a addToLayout is set to \a true (default),
      the action is added to the default vertical layout, i.e. subsequent
      calls of this function will create several ActionLabels arranged vertically,
      one below another.

      Set \a addToLayout to \a false if you want to add the action to the specified layout manually.
      This allows to do custom actions arrangements, i.e. horizontal etc.

      If \a addStretch is set to \a true (default),
      ActionLabel will be automatically aligned to the left side of the ActionGroup.
      Set \a addStretch to \a false if you want ActionLabel to occupy all the horizontal space.
      */
    ActionLabel* addAction(QAction *action, bool addToLayout = true, bool addStretch = true);

    /** Adds \a label to the group.

      \sa addAction() for the description.
      */
    ActionLabel* addActionLabel(ActionLabel *label, bool addToLayout = true, bool addStretch = true);

    /** Adds \a widget to the group. Returns \a true if it has been added successfully.

      \sa addAction() for the description.
      */
    bool addWidget(QWidget *widget, bool addToLayout = true, bool addStretch = true);

    /** Returns group's layout (QVBoxLayout by default).
      */
    QBoxLayout* groupLayout();

    /** Sets the scheme of the panel and all the child groups to \a scheme.

        By default, ActionPanelScheme::defaultScheme() is used.
      */
    void setScheme(ActionPanelScheme *pointer);

    /** Returns \a true if the group is expandable.

      \sa setExpandable().
      */
    bool isExpandable() const;

    /** Returns \a true if the group has header.

      \sa setHeader().
      */
    bool hasHeader() const;

    /** Returns text of the header.
        Only valid if the group has header (see hasHeader()).

      \sa setHeaderText().
      */
    QString headerText() const;

    QSize minimumSizeHint() const override;

public Q_SLOTS:
    /** Expands/collapses the group.
        Only valid if the group has header (see hasHeader()).
      */
    void showHide();

    /** Makes the group expandable if \a expandable is set to \a true.

      \sa isExpandable().
      */
    void setExpandable(bool expandable = true);

    /** Enables/disables group's header according to \a enable.

      \sa hasHeader().
      */
    void setHeader(bool enable = true);

    /** Sets text of the header to \a title.
        Only valid if the group has header (see hasHeader()).

      \sa headerText().
      */
    void setHeaderText(const QString & title);

protected Q_SLOTS:
    void processHide();
    void processShow();

protected:
    void init(bool header);

    void paintEvent ( QPaintEvent * event ) override;

    double m_foldStep, m_foldDelta, m_fullHeight, m_tempHeight;
    int m_foldDirection;

    QPixmap m_foldPixmap;

    class TaskHeader *myHeader;
    class TaskGroup *myGroup;
    QWidget *myDummy;

    ActionPanelScheme *myScheme;
};


} // namespace

#endif // ACTIONGROUP_H
