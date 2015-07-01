#ifndef ACTIONPANEL_H
#define ACTIONPANEL_H

#include <QFrame>


namespace QSint
{


class ActionPanelScheme;
class ActionGroup;


/**
    \brief Class representing panels of actions similar to Windows XP task panels.
    \since 0.2

    \image html ActionPanel1.png An example of ActionPanel

    ActionPanel acts like a container for ActionGroup which in turn are containers for
    the actions represented by ActionLabel.

    The look and fill is complete styleable via setScheme().
    Currently the following schemes available: ActionPanelScheme (the default),
    WinXPPanelScheme and WinXPPanelScheme2 (blue Windows XP schemes),
    WinVistaPanelScheme (Windows Vista variation), MacPanelScheme (MacOS variation),
    AndroidPanelScheme (Android variation).
*/
class ActionPanel : public QFrame
{
    typedef QFrame BaseClass;

    Q_OBJECT

public:
    /** Constructor.
      */
    explicit ActionPanel(QWidget *parent = 0);

    /** Adds a widget \a w to the ActionPanel's vertical layout.
      */
    void addWidget(QWidget *w);

    /** Adds a spacer with width \a s to the ActionPanel's vertical layout.
        Normally you should do this after all the ActionGroups were added, in order to
        maintain some space below.
      */
    void addStretch(int s = 0);

    /** Creates and adds to the ActionPanel's vertical layout an empty ActionGroup without header.
      */
    ActionGroup* createGroup();

    /** Creates and adds to the ActionPanel's vertical layout an empty ActionGroup with header's
        text set to \a title, but with no icon.

        If \a expandable set to \a true (default), the group can be expanded/collapsed by the user.
      */
    ActionGroup* createGroup(const QString &title, bool expandable = true);

    /** Creates and adds to the ActionPanel's vertical layout an empty ActionGroup with header's
        text set to \a title and icon set to \a icon.

        If \a expandable set to \a true (default), the group can be expanded/collapsed by the user.
      */
    ActionGroup* createGroup(const QPixmap &icon, const QString &title, bool expandable = true);

    /** Sets the scheme of the panel and all the child groups to \a scheme.

        By default, ActionPanelScheme::defaultScheme() is used.
      */
    void setScheme(ActionPanelScheme *scheme);

    virtual QSize minimumSizeHint() const;

protected:
    //virtual void paintEvent ( QPaintEvent * event );

    ActionPanelScheme *myScheme;
};


} // namespace

#endif // ACTIONPANEL_H
