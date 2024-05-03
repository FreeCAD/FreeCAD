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

#include "Action.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "PreferencePages/DlgSettingsWorkbenchesImp.h"
#include "DlgPreferencesImp.h"
#include "MainWindow.h"
#include "WorkbenchManager.h"
#include "WorkbenchSelector.h"


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

    auto layout = new QHBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabBar);
    layout->addWidget(moreButton);

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

        tabBar->setShape(orientation == "North" ? QTabBar::RoundedNorth :
            orientation == "South" ? QTabBar::RoundedSouth :
            orientation == "East" ? QTabBar::RoundedEast :
            QTabBar::RoundedWest);
    }

    tabBar->setDocumentMode(true);
    tabBar->setUsesScrollButtons(true);
    tabBar->setDrawBase(true);
    tabBar->setIconSize(QSize(16, 16));

    refreshList(aGroup->getEnabledWbActions());
 
    connect(aGroup, &WorkbenchGroup::workbenchListRefreshed, this, &WorkbenchTabWidget::refreshList);
    connect(aGroup->groupAction(), &QActionGroup::triggered, this, [this](QAction* action) {
        if (wbActionGroup->getDisabledWbActions().contains(action)) {
            if (additionalWorkbenchAction == action) {
                return;
            }

            if (additionalWorkbenchAction) {
                tabBar->removeTab(tabBar->count() - 1);
            }

            additionalWorkbenchAction = action;

            addWorkbenchTab(action);
            tabBar->setCurrentIndex(tabBar->count() - 1);

            return;
        }

        tabBar->setCurrentIndex(actionToTabIndex[action]);
    });

    connect(tabBar, qOverload<int>(&QTabBar::currentChanged), aGroup, [this](int index) {
        tabIndexToAction[index]->trigger();

        if (index != tabBar->count() - 1 && additionalWorkbenchAction) {
            tabBar->removeTab(tabBar->count() - 1);
            additionalWorkbenchAction = nullptr;
        }
    });

    if (parent->inherits("QToolBar")) {
        // Connect toolbar orientation changed
        QToolBar* tb = qobject_cast<QToolBar*>(parent);
        connect(tb, &QToolBar::topLevelChanged, this, &WorkbenchTabWidget::updateLayoutAndTabOrientation);
    }
}

void WorkbenchTabWidget::refreshList(QList<QAction*> actionList)
{
    actionToTabIndex.clear();

    // tabs->clear() (QTabBar has no clear)
    for (int i = tabBar->count() - 1; i >= 0; --i) {
        tabBar->removeTab(i);
    }

    for (QAction* action : actionList) {
        addWorkbenchTab(action);
    }

    if (additionalWorkbenchAction != nullptr) {
        addWorkbenchTab(additionalWorkbenchAction);
    }

    buildPrefMenu();
}

void WorkbenchTabWidget::addWorkbenchTab(QAction* action)
{
    ParameterGrp::handle hGrp;
    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");
    int itemStyleIndex = hGrp->GetInt("WorkbenchSelectorItem", 0);

    actionToTabIndex[action] = tabBar->count();
    tabIndexToAction[tabBar->count()] = action;

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

    tabBar->setTabToolTip(tabBar->count() - 1, action->toolTip());

    if (action->isChecked()) {
        tabBar->setCurrentIndex(tabBar->count() - 1);
    }
}

void WorkbenchTabWidget::updateLayoutAndTabOrientation(bool floating)
{
    auto parent = parentWidget();
    if (!parent || !parent->inherits("QToolBar")) {
        return;
    }

    ParameterGrp::handle hGrp = App::GetApplication()
        .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");

    Qt::ToolBarArea area;
    parent = parent->parentWidget();

    if (floating) {
        area = Qt::TopToolBarArea;
    }
    else if (parent && parent->parentWidget() == getMainWindow()->statusBar()) {
        area = Qt::BottomToolBarArea;
    }
    else if (parent && parent->parentWidget() == getMainWindow()->menuBar()) {
        area = Qt::TopToolBarArea;
    }
    else {
        QToolBar* tb = qobject_cast<QToolBar*>(parentWidget());
        area = getMainWindow()->toolBarArea(tb);
    }

    if (area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea) {
        tabBar->setShape(area == Qt::LeftToolBarArea ? QTabBar::RoundedWest : QTabBar::RoundedEast);
        hGrp->SetASCII("TabBarOrientation", area == Qt::LeftToolBarArea ? "West" : "East");
    }
    else {
        tabBar->setShape(area == Qt::TopToolBarArea ? QTabBar::RoundedNorth : QTabBar::RoundedSouth);
        hGrp->SetASCII("TabBarOrientation", area == Qt::TopToolBarArea ? "North" : "South");
    }
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

#include "moc_WorkbenchSelector.cpp"
