// SPDX-License-Identifier: LGPL-3.0-only
/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "actionlabel.h"

#include <QLabel>
#include <QVBoxLayout>


namespace QSint
{
/**
 * @brief A panel of actions, similar to Windows Vista/7 control panel items.
 *
 * An ActionBox displays an icon, a clickable header, and a list of actions.
 * Actions can have icons, tooltips, status tips, and support click/check functionality
 * (similar to ActionLabel).  Customizable via CSS.
 */
class QSINT_EXPORT ActionBox : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(QPixmap icon READ icon WRITE setIcon) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(ActionLabel header READ header) // clazy:exclude=qproperty-without-notify

public:
    /**
     * @brief Constructs an ActionBox.
     * @param parent The parent widget.
     */
    explicit ActionBox(QWidget *parent = nullptr);

    /**
     * @brief Constructs an ActionBox with a header text.
     * @param headerText The header text.
     * @param parent The parent widget.
     */
    ActionBox(const QString & headerText, QWidget *parent = nullptr);

    /**
     * @brief Constructs an ActionBox with an icon and header text.
     * @param icon The icon.
     * @param headerText The header text.
     * @param parent The parent widget.
     */
    explicit ActionBox(const QPixmap & icon, const QString & headerText, QWidget *parent = nullptr);

    /**
     * @brief Sets the ActionBox icon.
     * @param icon The icon.
     */
    void setIcon(const QPixmap & icon);

    /**
     * @brief Returns the ActionBox icon.
     * @return The icon.
     */
    QPixmap icon() const;

    /**
     * @brief Returns the header label.
     * @return The header label.
     */
    inline ActionLabel* header() const { return headerLabel; }

    /**
     * @brief Creates and adds an action from a QAction.
     * @param action The QAction.
     * @param l Optional layout to add the action to.  Defaults to the
     *          ActionBox's default vertical layout.
     * @return The created ActionLabel.
     */
    ActionLabel* createItem(QAction * action, QLayout * l = nullptr);

    /**
     * @brief Creates and adds multiple actions from a list of QActions.
     * @param actions The list of QActions.
     * @return The list of created ActionLabels.
     */
    QList<ActionLabel*> createItems(const QList<QAction*> actions);

    /**
     * @brief Creates and adds an action with text.
     * @param text The action text.
     * @param l Optional layout to add the action to.
     * @return The created ActionLabel.
     */
    ActionLabel* createItem(const QString & text = QString(), QLayout * l = nullptr);

    /**
     * @brief Creates and adds an action with an icon and text.
     * @param icon The action icon.
     * @param text The action text.
     * @param l Optional layout to add the action to.
     * @return The created ActionLabel.
     */
    ActionLabel* createItem(const QPixmap & icon, const QString & text, QLayout * l = nullptr);

    /**
     * @brief Creates and adds a spacer.
     * @param l Optional layout to add the spacer to. Defaults to the
     *          ActionBox's default vertical layout.
     * @return The created spacer item.
     */
    QSpacerItem* createSpacer(QLayout * l = nullptr);

    /**
     * @brief Creates a horizontal layout.
     * @return The created layout.
     */
    QLayout* createHBoxLayout();

    /**
     * @brief Returns the default layout used for actions.
     * @return The default layout.
     */
    inline QLayout* itemLayout() const { return dataLayout; }

    /**
     * @brief Adds a layout.
     * @param l The layout to add.
     */
    void addLayout(QLayout * l);

    /**
     * @brief Adds a widget.
     * @param w The widget to add.
     * @param l Optional layout to add the widget to. Defaults to the
     *          ActionBox's default vertical layout.
     */
    void addWidget(QWidget * w, QLayout * l = nullptr);

    /**
     * @brief Returns the recommended minimum size.
     * @return The minimum size hint.
     */
    QSize minimumSizeHint() const override;

protected:
    /**
     * @brief Initializes the ActionBox.
     * @param headerText The initial header text.
     */
    void init(const QString &headerText = QString());

    QVBoxLayout *dataLayout;       ///< Default layout for actions/widgets.
    QLabel *iconLabel;             ///< Label for the ActionBox icon.
    ActionLabel *headerLabel;      ///< Label for the header.
};
} // namespace
