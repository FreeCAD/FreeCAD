// SPDX-License-Identifier: LGPL-3.0-only
/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#pragma once

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

    Q_PROPERTY(bool expandable READ isExpandable WRITE setExpandable NOTIFY expandableChanged)
    Q_PROPERTY(bool header READ hasHeader WRITE setHeader NOTIFY headerChanged)
    Q_PROPERTY(QString headerText READ headerText WRITE setHeaderText NOTIFY headerTextChanged)

Q_SIGNALS:
    void expandableChanged();
    void headerChanged();
    void headerTextChanged();


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
     * @brief Creates and adds an action.
     * @param action The QAction to add.
     * @param addToLayout If `true` (default), adds the action to the group's layout.
     * @param addStretch If `true` (default), aligns the ActionLabel to the left.
     * @return The newly created ActionLabel.
     */
    ActionLabel* addAction(QAction *action, bool addToLayout = true, bool addStretch = true);

    /**
     * @brief Adds an existing ActionLabel.
     * @param label The ActionLabel to add.
     * @param addToLayout If `true` (default), adds the label to the group's layout.
     * @param addStretch If `true` (default), aligns the ActionLabel to the left.
     * @return The added ActionLabel.
     */
    ActionLabel* addActionLabel(ActionLabel *label, bool addToLayout = true, bool addStretch = true);

    /**
     * @brief Adds a widget to the group.
     * @param widget The widget to add.
     * @param addToLayout If `true` (default), adds the widget to the group's layout.
     * @param addStretch If `true` (default), aligns the widget to the left.
     * @return `true` if added successfully.
     */
    bool addWidget(QWidget *widget, bool addToLayout = true, bool addStretch = true);

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
     * @brief Checks if the group is expandable.
     * @return `true` if the group is expandable, `false` otherwise.
     */
    bool isExpandable() const;

    /**
     * @brief Sets whether the group is expandable.
     * @param expandable If `true`, the group can be expanded/collapsed.
     */
    void setExpandable(bool expandable);

    /**
     * @brief Checks if the group has a header.
     * @return `true` if the group has a header, `false` otherwise.
     */
    bool hasHeader() const;

    /**
     * @brief Sets whether the group has a header.
     * @param enable If `true`, the group will have a header.
     */
    void setHeader(bool enable);

    /**
     * @brief Returns the header text.
     * @return The header text.
     */
    QString headerText() const;

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
