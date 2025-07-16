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
# include <QTimer>
#endif

#include "Base/Tools.h"
#include "Action.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Dialogs/DlgPreferencesImp.h"
#include "MainWindow.h"
#include "WorkbenchSelector.h"
#include "ToolBarAreaWidget.h"


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

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");

    auto itemStyle = static_cast<WorkbenchItemStyle>(hGrp->GetInt("WorkbenchSelectorItem", 0));

    for (QAction* action : actionList) {
        QIcon icon = action->icon();

        if (icon.isNull() || itemStyle == WorkbenchItemStyle::TextOnly) {
            addItem(action->text());
        }
        else if (itemStyle == WorkbenchItemStyle::IconOnly) {
            addItem(icon, {}); // empty string to ensure that only icon is displayed
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
    setObjectName(QStringLiteral("WbTabBar"));

    tabBar = new WbTabBar(this);
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
    moreButton->setObjectName(QStringLiteral("WbTabBarMore"));

    if (parent->inherits("QToolBar")) {
        // when toolbar is created it is not yet placed in its designated area
        // therefore we need to wait a bit and then update layout when it is ready
        // this is prone to race conditions, but Qt does not supply any event that
        // informs us about toolbar changing its placement.
        //
        // previous implementation saved that information to user settings and
        // restored last layout but this creates issues when default workbench has
        // different layout than last visited one
        QTimer::singleShot(500, [this]() { updateLayout(); });
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

inline Qt::LayoutDirection WorkbenchTabWidget::direction() const
{
    return _direction;
}

void WorkbenchTabWidget::setDirection(Qt::LayoutDirection direction)
{
    _direction = direction;

    Q_EMIT directionChanged(direction);
}

inline int WorkbenchTabWidget::temporaryWorkbenchTabIndex() const
{
    if (direction() == Qt::RightToLeft) {
        return 0;
    }

    int nextTabIndex = tabBar->count();

    return temporaryWorkbenchAction ? nextTabIndex - 1 : nextTabIndex;
}

QAction* WorkbenchTabWidget::workbenchActivateActionByTabIndex(int tabIndex) const
{
    if (temporaryWorkbenchAction && tabIndex == temporaryWorkbenchTabIndex()) {
        return temporaryWorkbenchAction;
    }

    auto it = tabIndexToAction.find(tabIndex);

    if (it != tabIndexToAction.end()) {
        return it->second;
    }

    return nullptr;
}

int WorkbenchTabWidget::tabIndexForWorkbenchActivateAction(QAction* workbenchActivateAction) const
{
    if (workbenchActivateAction == temporaryWorkbenchAction) {
        return temporaryWorkbenchTabIndex();
    }

    return actionToTabIndex.at(workbenchActivateAction);
}

WorkbenchItemStyle Gui::WorkbenchTabWidget::itemStyle() const
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");
    return static_cast<WorkbenchItemStyle>(hGrp->GetInt("WorkbenchSelectorItem", 0));
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

    tabBar->setSelectionBehaviorOnRemove(
        direction() == Qt::LeftToRight
            ? QTabBar::SelectLeftTab
            : QTabBar::SelectRightTab
    );
}

void WorkbenchTabWidget::handleWorkbenchSelection(QAction* selectedWorkbenchAction)
{
    if (wbActionGroup->getDisabledWbActions().contains(selectedWorkbenchAction)) {
        if (temporaryWorkbenchAction == selectedWorkbenchAction) {
            return;
        }

        setTemporaryWorkbenchTab(selectedWorkbenchAction);
    }

    updateLayout();

    tabBar->setCurrentIndex(tabIndexForWorkbenchActivateAction(selectedWorkbenchAction));
}

void WorkbenchTabWidget::setTemporaryWorkbenchTab(QAction* workbenchActivateAction)
{
    auto temporaryTabIndex = temporaryWorkbenchTabIndex();

    if (temporaryWorkbenchAction) {
        temporaryWorkbenchAction = nullptr;
        tabBar->removeTab(temporaryTabIndex);
    }

    temporaryWorkbenchAction = workbenchActivateAction;

    if (!workbenchActivateAction) {
        return;
    }

    addWorkbenchTab(workbenchActivateAction, temporaryTabIndex);

    adjustSize();
}

void WorkbenchTabWidget::handleTabChange(int selectedTabIndex)
{
    // Prevents from rapid workbench changes on initialization as this can cause
    // some serious race conditions.
    if (isInitializing) {
        return;
    }

    if (auto workbenchActivateAction = workbenchActivateActionByTabIndex(selectedTabIndex)) {
        workbenchActivateAction->trigger();
    }

    if (selectedTabIndex != temporaryWorkbenchTabIndex()) {
        setTemporaryWorkbenchTab(nullptr);
    }

    adjustSize();
}

void WorkbenchTabWidget::updateWorkbenchList()
{
    if (isInitializing) {
        return;
    }

    tabBar->setItemStyle(itemStyle());

    // As clearing and adding tabs can cause changing current tab in QTabBar.
    // This in turn will cause workbench to change, so we need to prevent
    // processing of such events until the QTabBar is fully prepared.
    Base::StateLocker lock(isInitializing);

    actionToTabIndex.clear();
    tabIndexToAction.clear();

    // tabs->clear() (QTabBar has no clear)
    for (int i = tabBar->count() - 1; i >= 0; --i) {
        tabBar->removeTab(i);
    }

    for (QAction* action : wbActionGroup->getEnabledWbActions()) {
        addWorkbenchTab(action);
    }

    if (temporaryWorkbenchAction != nullptr) {
        setTemporaryWorkbenchTab(temporaryWorkbenchAction);
    }

    buildPrefMenu();
    adjustSize();
}

int WorkbenchTabWidget::addWorkbenchTab(QAction* action, int tabIndex)
{
    auto itemStyle = this->itemStyle();

    // if tabIndex is negative we assume that tab must be placed at the end of tabBar (default behavior)
    if (tabIndex < 0) {
        tabIndex = tabBar->count();
    }

    // for the maps we consider order in which tabs have been added
    // that's why here we use tabBar->count() instead of tabIndex
    actionToTabIndex[action] = tabBar->count();
    tabIndexToAction[tabBar->count()] = action;

    QIcon icon = action->icon();
    if (icon.isNull() || itemStyle == WorkbenchItemStyle::TextOnly) {
        tabBar->insertTab(tabIndex, action->text());
    }
    else if (itemStyle == WorkbenchItemStyle::IconOnly) {
        tabBar->insertTab(tabIndex, icon, {}); // empty string to ensure only icon is displayed
    }
    else {
        tabBar->insertTab(tabIndex, icon, action->text());
    }

    tabBar->setTabToolTip(tabIndex, action->toolTip());

    if (action->isChecked()) {
        tabBar->setCurrentIndex(tabIndex);
    }

    return tabIndex;
}

void WorkbenchTabWidget::setToolBarArea(Gui::ToolBarArea area)
{
    switch (area) {
        case Gui::ToolBarArea::LeftToolBarArea:
        case Gui::ToolBarArea::RightToolBarArea: {
            setDirection(Qt::LeftToRight);
            layout->setDirection(direction() == Qt::LeftToRight ? QBoxLayout::TopToBottom : QBoxLayout::BottomToTop);
            tabBar->setShape(area == Gui::ToolBarArea::LeftToolBarArea ? QTabBar::RoundedWest : QTabBar::RoundedEast);
            break;
        }

        case Gui::ToolBarArea::TopToolBarArea:
        case Gui::ToolBarArea::BottomToolBarArea:
        case Gui::ToolBarArea::LeftMenuToolBarArea:
        case Gui::ToolBarArea::RightMenuToolBarArea:
        case Gui::ToolBarArea::StatusBarToolBarArea: {
            bool isTop =
                area == Gui::ToolBarArea::TopToolBarArea ||
                area == Gui::ToolBarArea::LeftMenuToolBarArea ||
                area == Gui::ToolBarArea::RightMenuToolBarArea;

            bool isRightAligned =
                area == Gui::ToolBarArea::RightMenuToolBarArea ||
                area == Gui::ToolBarArea::StatusBarToolBarArea;

            setDirection(isRightAligned ? Qt::RightToLeft : Qt::LeftToRight);
            layout->setDirection(direction() == Qt::LeftToRight ? QBoxLayout::LeftToRight : QBoxLayout::RightToLeft);
            tabBar->setShape(isTop ? QTabBar::RoundedNorth : QTabBar::RoundedSouth);
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
        if (action->text() == QStringLiteral("<none>")) {
            continue;
        }

        menu->addAction(action);
    }

    menu->addSeparator();

    QAction* preferencesAction = menu->addAction(tr("Preferences"));
    connect(preferencesAction, &QAction::triggered, this, []() {
        Gui::Dialog::DlgPreferencesImp cDlg(getMainWindow());
        cDlg.activateGroupPage(QStringLiteral("Workbenches"), 0);
        cDlg.exec();
    });
}

void WorkbenchTabWidget::adjustSize()
{
    QWidget::adjustSize();

    parentWidget()->adjustSize();
}

#include "moc_WorkbenchSelector.cpp"
