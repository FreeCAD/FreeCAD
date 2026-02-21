// SPDX-License-Identifier: LGPL-3.0-only
/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef ACTIONGROUP_H
#define ACTIONGROUP_H

#include <QWidget>
#include <QBoxLayout>
#include <QTimer>
#include "qsint_global.h"


namespace QSint
{
class ActionLabel;
class ActionPanelScheme;
class TaskHeader;
class TaskGroup;

/**
 * @brief A collapsible group widget for organizing actions.
 *
 * An ActionGroup can have a header and contains actions (ActionLabels) or other widgets.
 */
class QSINT_EXPORT ActionGroup : public QWidget
{
    Q_OBJECT


public:
    /**
     * @brief Constructs an ActionGroup.
     * @param parent The parent widget.
     */
    explicit ActionGroup(QWidget *parent = nullptr);

    /**
     * @brief Constructs an ActionGroup with a title.
     * @param title The title of the group's header.
     * @param expandable If `true` (default), the group can be expanded/collapsed.
     * @param parent The parent widget.
     */
    explicit ActionGroup(const QString& title, bool expandable = true, QWidget *parent = nullptr);

    /**
     * @brief Constructs an ActionGroup with an icon and title.
     * @param icon The icon for the group's header.
     * @param title The title of the group's header.
     * @param expandable If `true` (default), the group can be expanded/collapsed.
     * @param parent The parent widget.
     */
    explicit ActionGroup(const QPixmap& icon, const QString& title, bool expandable = true, QWidget *parent = nullptr);

    /**
     * @brief Destroys the ActionGroup.
     */
    ~ActionGroup() override;

    /**
     * @brief Returns the group's layout.
     * @return The group's layout (QVBoxLayout by default).
     */
    QBoxLayout* groupLayout();

    /**
     * @brief Set the style of the widgets
     */
    void setScheme(ActionPanelScheme *scheme);

    /**
     * @brief Sets the header text.
     * @param text The header text.
     */
    void setHeaderText(const QString &text);

    /**
     * @brief Sets the header icon.
     * @param icon The header icon.
     */
    void setHeaderIcon(const QPixmap &icon);

    /**
     * @brief Returns the recommended minimum size for the group.
     * @return The minimum size hint.
     */
    QSize minimumSizeHint() const override;

public Q_SLOTS:
    /**
     * @brief Shows or hides the group's contents.
     */
    void showHide();

protected Q_SLOTS:
    /**
     * @brief Handles hiding the group's contents.
     */
    void processHide();

    /**
     * @brief Handles showing the group's contents.
     */
    void processShow();

protected:
    /**
     * @brief Paints the group.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Initializes the group.
     * @param hasHeader Whether the group has a header.
     */
    void init(bool hasHeader);

    double m_foldStep = 0;      ///< Current folding animation step.
    double m_foldDelta = 0;     ///< Change in height per animation step.
    double m_fullHeight = 0;    ///< Full (expanded) height of the group.
    double m_tempHeight = 0;    ///< Temporary height during animation.
    int m_foldDirection = 0;    ///< Direction of folding animation.

    QPixmap m_foldPixmap;       ///< Pixmap for the fold/unfold icon.

    TaskHeader *myHeader = nullptr;        ///< The group's header.
    TaskGroup *myGroup = nullptr;          ///< The container for actions/widgets.
    QWidget *myDummy = nullptr;            ///< Dummy widget for animation.
    ActionPanelScheme *myScheme = nullptr; ///< The color scheme.

private:
    const int separatorHeight = 1;
};
} // namespace QSint

#endif // ACTIONGROUP_H
