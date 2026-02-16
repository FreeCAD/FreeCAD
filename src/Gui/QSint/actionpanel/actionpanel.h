// SPDX-License-Identifier: LGPL-3.0-only
/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef ACTIONPANEL_H
#define ACTIONPANEL_H

#include <QFrame>
#include <QSpacerItem>
#include "qsint_global.h"


namespace QSint
{
class ActionPanelScheme;
class ActionGroup;

/**
 * @brief Provides a panel of actions, similar to Windows XP task panels.
 *
 * An ActionPanel contains ActionGroups, which in turn contain actions (represented by ActionLabels).
 */
class QSINT_EXPORT ActionPanel : public QFrame
{
    using BaseClass = QFrame;

    Q_OBJECT

public:
    /**
     * @brief Constructs an ActionPanel.
     * @param parent The parent widget.
     */
    explicit ActionPanel(QWidget *parent = nullptr);

    /**
     * @brief Adds a widget to the ActionPanel.
     * @param w The widget to add.
     */
    void addWidget(QWidget *w);

    /**
     * @brief Removes a widget from the ActionPanel.
     * @param w The widget to remove.
     */
    void removeWidget(QWidget *w);

    /// Adds a spacer to bottom of the ActionPanel.
    void addStretch();

    /// Removes the spacer from the ActionPanel  (if one was added).
    void removeStretch();

    /**
     * @brief Sets the color scheme for the panel and its child groups.
     * @param scheme The new scheme to use.  Defaults to `ActionPanelScheme::defaultScheme()`
     *               if not set.
     */
    void setScheme(ActionPanelScheme *scheme);

    /**
     * @brief Returns the recommended minimum size for the panel.
     * @return The minimum size hint.
     */
    QSize minimumSizeHint() const override;

protected:
    /// The color scheme used by the panel.
    ActionPanelScheme *myScheme;

    /// The spacer used for bottom spacing.
    QSpacerItem *mySpacer;
};

} // namespace

#endif // ACTIONPANEL_H
