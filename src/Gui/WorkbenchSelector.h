/***************************************************************************
 *   Copyright (c) 2024 Pierre-Louis Boyer <development[at]Ondsel.com>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 ***************************************************************************/


#pragma once

#include <QComboBox>
#include <QTabBar>
#include <QMenu>
#include <QToolButton>
#include <QLayout>
#include <QWheelEvent>

#include <FCGlobal.h>
#include <Gui/ToolBarManager.h>
#include <map>

namespace Gui
{
class WorkbenchGroup;

enum class WorkbenchItemStyle
{
    IconAndText = 0,
    IconOnly = 1,
    TextOnly = 2
};

class GuiExport WorkbenchComboBox: public QComboBox
{
Q_OBJECT  // NOLINT

    public: explicit WorkbenchComboBox(WorkbenchGroup* aGroup, QWidget* parent = nullptr);
    ~WorkbenchComboBox() override = default;
    WorkbenchComboBox(WorkbenchComboBox&& rhs) = delete;
    void showPopup() override;
    void setVisible(bool visible) override;

    WorkbenchComboBox operator=(WorkbenchComboBox&& rhs) = delete;

public Q_SLOTS:
    void refreshList(QList<QAction*>);

private:
    Q_DISABLE_COPY(WorkbenchComboBox)
};


class GuiExport WorkbenchTabWidget: public QWidget
{
    Q_OBJECT  // NOLINT
    Q_PROPERTY(Qt::LayoutDirection direction READ direction WRITE setDirection NOTIFY directionChanged)

        class WbTabBar: public QTabBar
    {
    public:
        explicit WbTabBar(QWidget* parent)
            : QTabBar(parent)
        {}

        QSize tabSizeHint(int index) const override
        {
            auto sizeFromParent = QTabBar::tabSizeHint(index);

            if (itemStyle() != WorkbenchItemStyle::IconOnly) {
                return sizeFromParent;
            }

            QStyleOptionTab opt;

            initStyleOption(&opt, index);

            int padding = style()->pixelMetric(QStyle::PM_TabBarTabHSpace, &opt, this);

            auto csz = iconSize();
            auto isHorizontal = opt.shape == RoundedNorth || opt.shape == RoundedSouth;

            if (isHorizontal) {
                csz.setWidth(csz.width() + padding);
            }
            else {
                csz.setHeight(csz.height() + padding);
            }

            auto size = style()->sizeFromContents(QStyle::CT_TabBarTab, &opt, csz, this);

            if (isHorizontal) {
                size.setHeight(sizeFromParent.height());
            }
            else {
                size.setWidth(sizeFromParent.width());
            }

            return size;
        }

        void wheelEvent(QWheelEvent* wheelEvent) override
        {
            // Qt does not expose any way to programmatically control scroll of QTabBar hence
            // we need to use a bit hacky solution of simulating clicks on the scroll buttons
            const auto buttonToClickName = wheelEvent->angleDelta().y() < 0
                ? QStringLiteral("ScrollLeftButton")
                : QStringLiteral("ScrollRightButton");

            // Qt introduces named buttons in Qt 6.3 and 5.15.6, before that they are not available
            if (const auto button = findChild<QAbstractButton*>(buttonToClickName)) {
                button->click();
            }
        }

        WorkbenchItemStyle itemStyle() const
        {
            return _itemStyle;
        }

        void setItemStyle(WorkbenchItemStyle itemStyle)
        {
            _itemStyle = itemStyle;
            setProperty("style", QString::fromUtf8(workbenchItemStyleToString(itemStyle)));
        }

    private:
        WorkbenchItemStyle _itemStyle {WorkbenchItemStyle::IconAndText};

        static const char* workbenchItemStyleToString(WorkbenchItemStyle style)
        {
            switch (style) {
                case WorkbenchItemStyle::IconAndText:
                    return "icon-and-text";
                case WorkbenchItemStyle::IconOnly:
                    return "icon-only";
                case WorkbenchItemStyle::TextOnly:
                    return "text-only";
                default:
                    return "WorkbenchItemStyle-internal-error";
            }
        }
    };

public:
    explicit WorkbenchTabWidget(WorkbenchGroup* aGroup, QWidget* parent = nullptr);

    void setToolBarArea(Gui::ToolBarArea area);
    void buildPrefMenu();

    Qt::LayoutDirection direction() const;
    void setDirection(Qt::LayoutDirection direction);

    void adjustSize();

public Q_SLOTS:
    void handleWorkbenchSelection(QAction* selectedWorkbenchAction);
    void handleTabChange(int selectedTabIndex);

    void updateLayout();
    void updateWorkbenchList();

Q_SIGNALS:
    void directionChanged(const Qt::LayoutDirection&);

protected:
    int addWorkbenchTab(QAction* workbenchActivateAction, int index = -1);

    void setTemporaryWorkbenchTab(QAction* workbenchActivateAction);
    int temporaryWorkbenchTabIndex() const;

    QAction* workbenchActivateActionByTabIndex(int tabIndex) const;
    int tabIndexForWorkbenchActivateAction(QAction* workbenchActivateAction) const;

    WorkbenchItemStyle itemStyle() const;

private:
    bool isInitializing = false;

    WorkbenchGroup* wbActionGroup;
    QToolButton* moreButton;
    WbTabBar* tabBar;
    QBoxLayout* layout;

    Qt::LayoutDirection _direction = Qt::LeftToRight;

    // this action is used for workbenches that are typically disabled
    QAction* temporaryWorkbenchAction = nullptr;

    std::map<QAction*, int> actionToTabIndex;
    std::map<int, QAction*> tabIndexToAction;
};


}  // namespace Gui
