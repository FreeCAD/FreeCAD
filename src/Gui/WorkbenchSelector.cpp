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
# include <QScreen>
# include <QToolBar>
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
    : QTabBar(parent)
    , wbActionGroup(aGroup)
{
    setToolTip(aGroup->toolTip());
    setStatusTip(aGroup->action()->statusTip());
    setWhatsThis(aGroup->action()->whatsThis());

    QAction* moreAction = new QAction(this);
    menu = new QMenu(this);
    moreAction->setMenu(menu);
    connect(moreAction, &QAction::triggered, [this]() {
        menu->popup(QCursor::pos());
    });
    connect(menu, &QMenu::aboutToHide, this, [this]() {
        // if the more tab did not triggered a disabled workbench, make sure we reselect the correct tab.
        std::string activeWbName = WorkbenchManager::instance()->activeName();
        for (int i = 0; i < count(); ++i) {
            if (wbActionGroup->actions()[i]->objectName().toStdString() == activeWbName) {
                setCurrentIndex(i);
                break;
            }
        }
    });

    if (parent->inherits("QToolBar")) {
        // set the initial orientation. We cannot do updateLayoutAndTabOrientation(false); 
        // because on init the toolbar area is always TopToolBarArea.
        ParameterGrp::handle hGrp;
        hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");
        std::string orientation = hGrp->GetASCII("TabBarOrientation", "North");

        this->setShape(orientation == "North" ? QTabBar::RoundedNorth :
            orientation == "South" ? QTabBar::RoundedSouth :
            orientation == "East" ? QTabBar::RoundedEast :
            QTabBar::RoundedWest);
    }

    setDocumentMode(true);
    setUsesScrollButtons(true);
    setDrawBase(true);
    setObjectName(QString::fromLatin1("WbTabBar"));
    setIconSize(QSize(16, 16));

    refreshList(aGroup->getEnabledWbActions());
    connect(aGroup, &WorkbenchGroup::workbenchListRefreshed, this, &WorkbenchTabWidget::refreshList);
    connect(aGroup->groupAction(), &QActionGroup::triggered, this, [this, aGroup](QAction* action) {
        int index = std::min(aGroup->actions().indexOf(action), this->count() - 1);
        setCurrentIndex(index);
    });
    connect(this, qOverload<int>(&QTabBar::tabBarClicked), aGroup, [aGroup, moreAction](int index) {
        if (index < aGroup->getEnabledWbActions().size()) {
            aGroup->actions()[index]->trigger();
        }
        else {
            moreAction->trigger();
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
    // tabs->clear() (QTabBar has no clear)
    for (int i = count() - 1; i >= 0; --i) {
        removeTab(i);
    }

    ParameterGrp::handle hGrp;
    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");
    int itemStyleIndex = hGrp->GetInt("WorkbenchSelectorItem", 0);

    for (QAction* action : actionList) {
        QIcon icon = action->icon();
        if (icon.isNull() || itemStyleIndex == 2) {
            addTab(action->text());
        }
        else if (itemStyleIndex == 1) {
            addTab(icon, QString::fromLatin1(""));
        }
        else {
            addTab(icon, action->text());
        }

        if (action->isChecked()) {
            setCurrentIndex(count() - 1);
        }
    }

    QIcon icon = Gui::BitmapFactory().iconFromTheme("list-add");
    if (itemStyleIndex == 2) {
        addTab(tr("More"));
    }
    else if (itemStyleIndex == 1) {
        addTab(icon, QString::fromLatin1(""));
    }
    else {
        addTab(icon, tr("More"));
    }
    
    buildPrefMenu();
}

void WorkbenchTabWidget::updateLayoutAndTabOrientation(bool floating)
{
    if (!parentWidget()->inherits("QToolBar") || floating) {
        return;
    }

    ParameterGrp::handle hGrp = App::GetApplication()
        .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");

    QToolBar* tb = qobject_cast<QToolBar*>(parentWidget());
    Qt::ToolBarArea area = getMainWindow()->toolBarArea(tb);

    if (area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea) {
        setShape(area == Qt::LeftToolBarArea ? QTabBar::RoundedWest : QTabBar::RoundedEast);
        hGrp->SetASCII("TabBarOrientation", area == Qt::LeftToolBarArea ? "West" : "East");
    }
    else {
        setShape(area == Qt::TopToolBarArea ? QTabBar::RoundedNorth : QTabBar::RoundedSouth);
        hGrp->SetASCII("TabBarOrientation", area == Qt::TopToolBarArea ? "North" : "South");
    }
}

void WorkbenchTabWidget::buildPrefMenu()
{
    menu->clear();

    // Add disabled workbenches, sorted alphabetically.
    menu->addActions(wbActionGroup->getDisabledWbActions());

    menu->addSeparator();

    QAction* preferencesAction = menu->addAction(tr("Preferences"));
    connect(preferencesAction, &QAction::triggered, this, [this]() {
        Gui::Dialog::DlgPreferencesImp cDlg(getMainWindow());
        cDlg.activateGroupPage(QString::fromUtf8("Workbenches"), 0);
        cDlg.exec();
    });
}

#include "moc_WorkbenchSelector.cpp"
