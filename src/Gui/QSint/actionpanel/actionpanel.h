// SPDX-License-Identifier: LGPL-3.0-only
/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#pragma once

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

    /**
     * @brief Adds a spacer to bottom of the ActionPanel.
     * @param s The width of the spacer..
     */
    void addStretch(int s = 0);

    /**
     * @brief Removes the spacer from the ActionPanel  (if one was added).
     */
    void removeStretch();

    /**
     * @brief Creates and adds an empty ActionGroup (without a header) to the panel.
     * @return The newly created ActionGroup.
     */
    ActionGroup* createGroup();

    /**
     * @brief Creates and adds an ActionGroup (with a header) to the panel.
     * @param title The title of the group's header.
     * @param expandable If `true` (default), the group can be expanded/collapsed.
     * @return The newly created ActionGroup.
     */
    ActionGroup* createGroup(const QString &title, bool expandable = true);

    /**
     * @brief Creates and adds an ActionGroup (with a header) to the panel.
     * @param icon The icon for the group's header.
     * @param title The title of the group's header.
     * @param expandable If `true` (default), the group can be expanded/collapsed.
     * @return The newly created ActionGroup.
     */
    ActionGroup* createGroup(const QPixmap &icon, const QString &title, bool expandable = true);

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
    /** @brief The color scheme used by the panel. */
    ActionPanelScheme *myScheme;

    /** @brief The spacer used for bottom spacing. */
    QSpacerItem *mySpacer;
};

} // namespace
