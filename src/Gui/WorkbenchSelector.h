/***************************************************************************
 *   Copyright (c) 2024 Pierre-Louis Boyer <development[at]Ondsel.com>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef GUI_WORKBENCHSELECTOR_H
#define GUI_WORKBENCHSELECTOR_H

#include <QComboBox>
#include <QTabBar>
#include <QMenu>
#include <QToolButton>
#include <QLayout>
#include <FCGlobal.h>
#include <Gui/ToolBarManager.h>
#include <map>

namespace Gui
{
class WorkbenchGroup;

enum WorkbenchItemStyle {
    IconAndText = 0,
    IconOnly = 1,
    TextOnly = 2
};

class GuiExport WorkbenchComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit WorkbenchComboBox(WorkbenchGroup* aGroup, QWidget* parent = nullptr);
    void showPopup() override;

public Q_SLOTS:
    void refreshList(QList<QAction*>);

private:
    Q_DISABLE_COPY(WorkbenchComboBox)
};


class GuiExport WorkbenchTabWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(Qt::LayoutDirection direction READ direction WRITE setDirection NOTIFY directionChanged)

    int addWorkbenchTab(QAction* workbenchActivateAction, int index = -1);

    void setTemporaryWorkbenchTab(QAction* workbenchActivateAction);
    int temporaryWorkbenchTabIndex() const;

    QAction* workbenchActivateActionByTabIndex(int tabIndex) const;
    int tabIndexForWorkbenchActivateAction(QAction* workbenchActivateAction) const;

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

private:
    bool isInitializing = false;

    WorkbenchGroup* wbActionGroup;
    QToolButton* moreButton;
    QTabBar* tabBar;
    QBoxLayout* layout;

    Qt::LayoutDirection _direction = Qt::LeftToRight;

    // this action is used for workbenches that are typically disabled
    QAction* temporaryWorkbenchAction = nullptr;

    std::map<QAction*, int> actionToTabIndex;
    std::map<int, QAction*> tabIndexToAction;
};



} // namespace Gui

#endif // GUI_WORKBENCHSELECTOR_H
