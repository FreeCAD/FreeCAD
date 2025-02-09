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
class TaskHeader;
class TaskGroup;

/**
 * @brief A collapsible group widget for organizing actions
 *
 * ActionGroup consists of an optional header and a collection of actions represented by ActionLabel.
 * It can also contain arbitrary widgets.
 *
 */
class QSINT_EXPORT ActionGroup : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool expandable READ isExpandable WRITE setExpandable)
    Q_PROPERTY(bool header READ hasHeader WRITE setHeader)
    Q_PROPERTY(QString headerText READ headerText WRITE setHeaderText)

public:
    explicit ActionGroup(QWidget *parent = nullptr);
    explicit ActionGroup(const QString& title, bool expandable = true, QWidget *parent = nullptr);
    explicit ActionGroup(const QPixmap& icon, const QString& title, bool expandable = true, QWidget *parent = nullptr);
    ~ActionGroup() override;
    /**
     * @brief Creates an action item from the given `action` and returns it.
     *
     * If `addToLayout` is `true` (default), the action is added to the default vertical layout, meaning
     * subsequent calls will arrange multiple `ActionLabel`s vertically, one below another.
     *
     * If `addToLayout` is `false`, the action must be added to a layout manually.
     * This allows for custom arrangements, such as horizontal layouts.
     *
     * If `addStretch` is `true` (default),`ActionLabel` will be automatically aligned to the left side.
     * if `addStretch` is `false` `ActionLabel` will occupy all available horizontal space.
     */
    ActionLabel* addAction(QAction *action, bool addToLayout = true, bool addStretch = true);

    /**
     * @brief Adds an `ActionLabel` to the group.
     * See `addAction()` for parameter details.
     */
    ActionLabel* addActionLabel(ActionLabel *label, bool addToLayout = true, bool addStretch = true);

    /**
     * @brief Adds a `QWidget` to the group. Returns `true` if added successfully.
     * See `addAction()` for parameter details.
     */
    bool addWidget(QWidget *widget, bool addToLayout = true, bool addStretch = true);

    /**
     * @brief Returns the group's layout (QVBoxLayout by default).
     */
    QBoxLayout* groupLayout();

    /**
     * @brief Checks if the group can collapse or expand.
     */
    bool isExpandable() const;

    /**
     * @brief Makes the group expandable or not.
     */
    void setExpandable(bool expandable);

    /**
     * @brief Checks if the group has a header.
     */
    bool hasHeader() const;

    /**
     * @brief Enables or disables the group's header.
     */
    void setHeader(bool enable);

    /**
     * @brief Returns the text of the header.
     */
    QString headerText() const;
    /**
     * @brief Sets the text of the header.
     */
    void setHeaderText(const QString &text);
    /**
     * @brief Sets the icon of the header.
     */
    void setHeaderIcon(const QPixmap &icon);

    QSize minimumSizeHint() const override;

    enum FoldEffect
    {
        NoFolding,
        ShrunkFolding,
        SlideFolding
    };

public Q_SLOTS:
    void showHide();

protected Q_SLOTS:
    void processHide();
    void processShow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void init(bool hasHeader);

    double m_foldStep = 0;
    double m_foldDelta = 0;
    double m_fullHeight = 0;
    double m_tempHeight = 0;
    int m_foldDirection = 0;

    QPixmap m_foldPixmap;

    std::unique_ptr<TaskHeader> myHeader;
    std::unique_ptr<TaskGroup> myGroup;
    std::unique_ptr<QWidget> myDummy;
};

} // namespace QSint

#endif // ACTIONGROUP_H
