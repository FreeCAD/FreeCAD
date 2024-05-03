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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QAbstractItemView>
# include <QActionGroup>
# include <QApplication>
# include <QMenuBar>
# include <QScreen>
# include <QStatusBar>
# include <QToolBar>
# include <QLayout>
#endif

#include "Base/Tools.h"
#include "Action.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "DlgPreferencesImp.h"
#include "MainWindow.h"
#include "WorkbenchSelector.h"
#include "ToolBarManager.h"


using namespace Gui;

WorkbenchComboBox::WorkbenchComboBox(WorkbenchGroup* aGroup, QWidget* parent) : QComboBox(parent)
{
    setIconSize(QSize(16, 16));
    setToolTip(aGroup->toolTip());
    setStatusTip(aGroup->action()->statusTip());
    setWhatsThis(aGroup->action()->whatsThis());
    refreshList(aGroup->getEnabledWbActions());
    connect(aGroup, &WorkbenchGroup::workbenchListRefreshed, this, &WorkbenchComboBox::refreshList);
    connect(aGroup->groupAction(), &QActionGroup::triggered, this, [this, aGroup](QAction* action) {
        setCurrentIndex(aGroup->actions().indexOf(action));
    });
    connect(this, qOverload<int>(&WorkbenchComboBox::activated), aGroup, [aGroup](int index) {
        aGroup->actions()[index]->trigger();
    });
}

void WorkbenchComboBox::showPopup()
{
    int rows = count();
    if (rows > 0) {
        int height = view()->sizeHintForRow(0);
        int maxHeight = QApplication::primaryScreen()->size().height();
        view()->setMinimumHeight(qMin(height * rows, maxHeight/2));
    }

    QComboBox::showPopup();
}

void WorkbenchComboBox::refreshList(QList<QAction*> actionList)
{
    clear();

    ParameterGrp::handle hGrp;
    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");
    int itemStyleIndex = hGrp->GetInt("WorkbenchSelectorItem", 0);

    for (QAction* action : actionList) {
        QIcon icon = action->icon();
        if (icon.isNull() || itemStyleIndex == 2) {
            addItem(action->text());
        }
        else if (itemStyleIndex == 1) {
            addItem(icon, QString::fromLatin1(""));
        }
        else {
            addItem(icon, action->text());
        }

        if (action->isChecked()) {
            this->setCurrentIndex(this->count() - 1);
        }
    }
}


WorkbenchTabWidget::WorkbenchTabWidget(WorkbenchGroup* aGroup, QWidget* parent)
    : QWidget(parent)
    , wbActionGroup(aGroup)
{
    setToolTip(aGroup->toolTip());
    setStatusTip(aGroup->action()->statusTip());
    setWhatsThis(aGroup->action()->whatsThis());
    setObjectName(QString::fromLatin1("WbTabBar"));

    tabBar = new QTabBar(this);
    moreButton = new QToolButton(this);

    layout = new QBoxLayout(QBoxLayout::LeftToRight, this);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabBar);
    layout->addWidget(moreButton);
    layout->setAlignment(moreButton, Qt::AlignCenter);

    setLayout(layout);

    moreButton->setIcon(Gui::BitmapFactory().iconFromTheme("list-add"));
    moreButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    moreButton->setPopupMode(QToolButton::InstantPopup);
    moreButton->setMenu(new QMenu(moreButton));
    moreButton->setObjectName(QString::fromLatin1("WbTabBarMore"));

    if (parent->inherits("QToolBar")) {
        // set the initial orientation. We cannot do updateLayoutAndTabOrientation(false);
        // because on init the toolbar area is always TopToolBarArea.
        ParameterGrp::handle hGrp;
        hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");
        std::string orientation = hGrp->GetASCII("TabBarOrientation", "North");

        setToolBarArea(
            orientation == "North" ? Gui::ToolBarArea::TopToolBarArea :
            orientation == "South" ? Gui::ToolBarArea::BottomToolBarArea :
            orientation == "East" ? Gui::ToolBarArea::LeftToolBarArea :
            Gui::ToolBarArea::RightToolBarArea
        );
    }

    tabBar->setDocumentMode(true);
    tabBar->setUsesScrollButtons(true);
    tabBar->setDrawBase(true);
    tabBar->setIconSize(QSize(16, 16));

    updateWorkbenchList();
 
    connect(aGroup, &WorkbenchGroup::workbenchListRefreshed, this, &WorkbenchTabWidget::updateWorkbenchList);
    connect(aGroup->groupAction(), &QActionGroup::triggered, this, &WorkbenchTabWidget::handleWorkbenchSelection);
    connect(tabBar, &QTabBar::currentChanged, this, &WorkbenchTabWidget::handleTabChange);

    if (auto toolBar = qobject_cast<QToolBar*>(parent)) {
        connect(toolBar, &QToolBar::topLevelChanged, this, &WorkbenchTabWidget::updateLayout);
        connect(toolBar, &QToolBar::orientationChanged, this, &WorkbenchTabWidget::updateLayout);
    }
}

void WorkbenchTabWidget::updateLayout() 
{
    if (!parentWidget()) {
        setToolBarArea(Gui::ToolBarArea::TopToolBarArea);
        return;
    }

    if (auto toolBar = qobject_cast<QToolBar*>(parentWidget())) {
        setSizePolicy(toolBar->sizePolicy());
        tabBar->setSizePolicy(toolBar->sizePolicy());

        if (toolBar->isFloating()) {
            setToolBarArea(toolBar->orientation() == Qt::Horizontal ? Gui::ToolBarArea::TopToolBarArea : Gui::ToolBarArea::LeftToolBarArea);
            return;
        }
    }

    auto toolBarArea = Gui::ToolBarManager::getInstance()->toolBarArea(parentWidget());

    setToolBarArea(toolBarArea);
}

void WorkbenchTabWidget::handleWorkbenchSelection(QAction* selectedWorkbenchAction) 
{
    if (wbActionGroup->getDisabledWbActions().contains(selectedWorkbenchAction)) {
        if (additionalWorkbenchAction == selectedWorkbenchAction) {
            return;
        }

        if (additionalWorkbenchAction) {
            tabBar->removeTab(tabBar->count() - 1);
        }

        additionalWorkbenchAction = selectedWorkbenchAction;

        addWorkbenchTab(selectedWorkbenchAction);
        tabBar->setCurrentIndex(tabBar->count() - 1);

        return;
    }

    updateLayout();

    tabBar->setCurrentIndex(actionToTabIndex[selectedWorkbenchAction]);
}

void WorkbenchTabWidget::handleTabChange(int selectedTabIndex)
{
    // Prevents from rapid workbench changes on initialization as this can cause
    // some serious race conditions.
    if (isInitializing) {
        return;
    }

    if (tabIndexToAction.find(selectedTabIndex) != tabIndexToAction.end()) {
        tabIndexToAction[selectedTabIndex]->trigger();
    }

    if (selectedTabIndex != tabBar->count() - 1 && additionalWorkbenchAction) {
        tabBar->removeTab(tabBar->count() - 1);
        additionalWorkbenchAction = nullptr;
    }

    adjustSize();
}

void WorkbenchTabWidget::updateWorkbenchList()
{
    // As clearing and adding tabs can cause changing current tab in QTabBar.
    // This in turn will cause workbench to change, so we need to prevent 
    // processing of such events until the QTabBar is fully prepared.
    Base::StateLocker lock(isInitializing);

    actionToTabIndex.clear();

    // tabs->clear() (QTabBar has no clear)
    for (int i = tabBar->count() - 1; i >= 0; --i) {
        tabBar->removeTab(i);
    }

    for (QAction* action : wbActionGroup->getEnabledWbActions()) {
        addWorkbenchTab(action);
    }

    if (additionalWorkbenchAction != nullptr) {
        addWorkbenchTab(additionalWorkbenchAction);
    }

    buildPrefMenu();
    adjustSize();
}

void WorkbenchTabWidget::addWorkbenchTab(QAction* action)
{
    ParameterGrp::handle hGrp;
    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");
    int itemStyleIndex = hGrp->GetInt("WorkbenchSelectorItem", 0);

    auto  tabIndex = tabBar->count();

    actionToTabIndex[action] = tabIndex;
    tabIndexToAction[tabIndex] = action;

    QIcon icon = action->icon();
    if (icon.isNull() || itemStyleIndex == 2) {
        tabBar->addTab(action->text());
    }
    else if (itemStyleIndex == 1) {
        tabBar->addTab(icon, QString::fromLatin1(""));
    }
    else {
        tabBar->addTab(icon, action->text());
    }

    tabBar->setTabToolTip(tabIndex, action->toolTip());

    if (action->isChecked()) {
        tabBar->setCurrentIndex(tabIndex);
    }
}

void WorkbenchTabWidget::setToolBarArea(Gui::ToolBarArea area)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");

    switch (area) {
        case Gui::ToolBarArea::LeftToolBarArea: 
        case Gui::ToolBarArea::RightToolBarArea: {
            layout->setDirection(QBoxLayout::TopToBottom);
            tabBar->setShape(area == Gui::ToolBarArea::LeftToolBarArea ? QTabBar::RoundedWest : QTabBar::RoundedEast);
            hGrp->SetASCII("TabBarOrientation", area == Gui::ToolBarArea::LeftToolBarArea ? "West" : "East");
            break;
        }

        case Gui::ToolBarArea::TopToolBarArea:
        case Gui::ToolBarArea::BottomToolBarArea:
        case Gui::ToolBarArea::LeftMenuToolBarArea:
        case Gui::ToolBarArea::RightMenuToolBarArea:
        case Gui::ToolBarArea::StatusBarToolBarArea: {
            layout->setDirection(QBoxLayout::LeftToRight);

            bool isTop = 
                area == Gui::ToolBarArea::TopToolBarArea ||
                area == Gui::ToolBarArea::LeftMenuToolBarArea ||
                area == Gui::ToolBarArea::RightMenuToolBarArea;

            tabBar->setShape(isTop ? QTabBar::RoundedNorth : QTabBar::RoundedSouth);
            hGrp->SetASCII("TabBarOrientation", isTop ? "North" : "South");
            break;
        }
        default:
            // no-op
            break;
    }

    adjustSize();
}

void WorkbenchTabWidget::buildPrefMenu()
{
    auto menu = moreButton->menu();

    menu->clear();

    // Add disabled workbenches, sorted alphabetically.
    for (auto action : wbActionGroup->getDisabledWbActions()) {
        if (action->text() == QString::fromLatin1("<none>")) {
            continue;
        }

        menu->addAction(action);
    }

    menu->addSeparator();

    QAction* preferencesAction = menu->addAction(tr("Preferences"));
    connect(preferencesAction, &QAction::triggered, this, []() {
        Gui::Dialog::DlgPreferencesImp cDlg(getMainWindow());
        cDlg.activateGroupPage(QString::fromUtf8("Workbenches"), 0);
        cDlg.exec();
    });
}

void WorkbenchTabWidget::adjustSize()
{
    QWidget::adjustSize();

    parentWidget()->adjustSize();
}

#include "moc_WorkbenchSelector.cpp"
