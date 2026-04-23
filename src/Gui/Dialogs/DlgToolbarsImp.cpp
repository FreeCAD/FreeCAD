// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QComboBox>
#include <QSignalBlocker>
#include <QToolBar>
#include <QToolButton>

#include "Dialogs/DlgToolbarsImp.h"
#include "Dialogs/DlgKeyboardImp.h"
#include "ui_DlgToolbars.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Action.h"
#include "ToolBarManager.h"
#include "MainWindow.h"
#include "ToolBarManager.h"
#include "Widgets.h"
#include "Workbench.h"
#include "WorkbenchManager.h"


using namespace Gui::Dialog;

namespace
{
QString customToolbarTierName(const QTreeWidgetItem* item)
{
    if (!item) {
        return {};
    }

    return item->data(2, Qt::UserRole).toString();
}
}  // namespace

/* TRANSLATOR Gui::Dialog::DlgCustomToolbars */

/**
 *  Constructs a DlgCustomToolbars which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgCustomToolbars::DlgCustomToolbars(DlgCustomToolbars::Type t, QWidget* parent)
    : CustomizeActionPage(parent)
    , ui(new Ui_DlgCustomToolbars)
    , type(t)
{
    ui->setupUi(this);
    setupConnections();

    ui->moveActionRightButton->setIcon(BitmapFactory().iconFromTheme("button_right"));
    ui->moveActionLeftButton->setIcon(BitmapFactory().iconFromTheme("button_left"));
    ui->moveActionDownButton->setIcon(BitmapFactory().iconFromTheme("button_down"));
    ui->moveActionUpButton->setIcon(BitmapFactory().iconFromTheme("button_up"));

    auto sepItem = new QTreeWidgetItem;
    sepItem->setText(1, tr("<Separator>"));
    sepItem->setData(1, Qt::UserRole, QByteArray("Separator"));
    sepItem->setSizeHint(0, QSize(32, 32));

    conn = DlgCustomKeyboardImp::initCommandWidgets(
        ui->commandTreeWidget,
        sepItem,
        ui->categoryBox,
        ui->editCommand
    );

    // fills the combo box with all available workbenches
    QStringList workbenches = Application::Instance->workbenches();
    workbenches.sort();
    int index = 1;
    ui->workbenchBox->addItem(QApplication::windowIcon(), tr("Global"));
    ui->workbenchBox->setItemData(0, QVariant(QStringLiteral("Global")), Qt::UserRole);
    for (const auto& workbench : workbenches) {
        QPixmap px = Application::Instance->workbenchIcon(workbench);
        QString mt = Application::Instance->workbenchMenuText(workbench);
        if (mt != QLatin1String("<none>")) {
            if (px.isNull()) {
                ui->workbenchBox->addItem(mt);
            }
            else {
                ui->workbenchBox->addItem(px, mt);
            }
            ui->workbenchBox->setItemData(index, QVariant(workbench), Qt::UserRole);
            index++;
        }
    }

    updateToolbarTreeHeaders();

    Workbench* w = WorkbenchManager::instance()->active();
    if (w) {
        QString name = QString::fromLatin1(w->name().c_str());
        int index = ui->workbenchBox->findData(name);
        ui->workbenchBox->setCurrentIndex(index);
    }
    onWorkbenchBoxActivated(ui->workbenchBox->currentIndex());
}

/** Destroys the object and frees any allocated resources */
DlgCustomToolbars::~DlgCustomToolbars() = default;

void DlgCustomToolbars::setupConnections()
{
    // clang-format off
    connect(ui->workbenchBox, qOverload<int>(&QComboBox::activated),
            this, &DlgCustomToolbars::onWorkbenchBoxActivated);
    connect(ui->moveActionRightButton, &QPushButton::clicked,
            this, &DlgCustomToolbars::onMoveActionRightButtonClicked);
    connect(ui->moveActionLeftButton, &QPushButton::clicked,
            this, &DlgCustomToolbars::onMoveActionLeftButtonClicked);
    connect(ui->moveActionUpButton, &QPushButton::clicked,
            this, &DlgCustomToolbars::onMoveActionUpButtonClicked);
    connect(ui->moveActionDownButton, &QPushButton::clicked,
            this, &DlgCustomToolbars::onMoveActionDownButtonClicked);
    connect(ui->newButton, &QPushButton::clicked,
            this, &DlgCustomToolbars::onNewButtonClicked);
    connect(ui->renameButton, &QPushButton::clicked,
            this, &DlgCustomToolbars::onRenameButtonClicked);
    connect(ui->deleteButton, &QPushButton::clicked,
            this, &DlgCustomToolbars::onDeleteButtonClicked);
    connect(ui->resetLayoutButton, &QPushButton::clicked,
            this, &DlgCustomToolbars::onResetLayoutButtonClicked);
    // clang-format on

    if (auto* manager = ToolBarManager::getInstance()) {
        connect(
            manager,
            &ToolBarManager::toolbarLayoutContextChanged,
            this,
            &DlgCustomToolbars::updateToolbarLayoutControls
        );
    }
    if (auto* mainWindow = MainWindow::getInstance()) {
        connect(mainWindow, &MainWindow::workbenchActivatedCompleted, this, [this] {
            updateToolbarLayoutControls();
        });
    }
}

void DlgCustomToolbars::addCustomToolbar(const QString&)
{}

void DlgCustomToolbars::removeCustomToolbar(const QString&)
{}

void DlgCustomToolbars::renameCustomToolbar(const QString&, const QString&)
{}

void DlgCustomToolbars::addCustomCommand(const QString&, const QByteArray&)
{}

void DlgCustomToolbars::removeCustomCommand(const QString&, const QByteArray&)
{}

void DlgCustomToolbars::moveUpCustomCommand(const QString&, const QByteArray&)
{}

void DlgCustomToolbars::moveDownCustomCommand(const QString&, const QByteArray&)
{}

void DlgCustomToolbars::hideEvent(QHideEvent* event)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());

    CustomizeActionPage::hideEvent(event);
}

void DlgCustomToolbars::showEvent(QShowEvent* event)
{
    updateToolbarLayoutControls();
    CustomizeActionPage::showEvent(event);
}

void DlgCustomToolbars::onActivateCategoryBox()
{}

// called from DlgMacroExecuteImp toolbar walkthrough function
void DlgCustomToolbars::activateWorkbenchBox(int index)
{
    onWorkbenchBoxActivated(index);
}

void DlgCustomToolbars::onWorkbenchBoxActivated(int index)
{
    QVariant data = ui->workbenchBox->itemData(index, Qt::UserRole);
    QString workbench = data.toString();
    ui->toolbarTreeWidget->clear();

    QByteArray workbenchname = workbench.toLatin1();
    importCustomToolbars(workbenchname);
    updateToolbarLayoutControls();
}

void DlgCustomToolbars::updateToolbarTreeHeaders()
{
    QStringList labels;
    labels << (type == Toolbar ? tr("Toolbar") : tr("Toolbox Bar")) << tr("Scope");
    if (type == Toolbar) {
        labels << tr("Tier");
    }
    ui->toolbarTreeWidget->setHeaderLabels(labels);
    ui->toolbarTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->toolbarTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    if (type == Toolbar) {
        ui->toolbarTreeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    }
}

QString DlgCustomToolbars::customToolbarPersistenceKey(
    const QString& toolbarName,
    const QString& workbench
) const
{
    if (toolbarName.isEmpty()) {
        return {};
    }

    if (workbench == QLatin1String("Global")) {
        return ToolBarManager::makeToolBarPersistenceKey(QStringLiteral("global"), {}, toolbarName);
    }

    if (workbench.isEmpty()) {
        return ToolBarManager::makeToolBarPersistenceKey({}, {}, toolbarName);
    }

    return ToolBarManager::makeToolBarPersistenceKey(QStringLiteral("wb"), workbench, toolbarName);
}

Gui::ToolBarItem::Tier DlgCustomToolbars::customToolbarTier(const QTreeWidgetItem* item) const
{
    if (type != Toolbar) {
        return ToolBarItem::Tier::Secondary;
    }

    return ToolBarManager::customToolBarTierFromName(customToolbarTierName(item));
}

void DlgCustomToolbars::setCustomToolbarTier(QTreeWidgetItem* item, ToolBarItem::Tier tier)
{
    if (!item || item->parent() || type != Toolbar) {
        return;
    }

    const auto normalizedTier = ToolBarManager::normalizeCustomToolBarTier(tier);
    const auto tierName = ToolBarManager::toolBarTierName(normalizedTier);
    item->setData(2, Qt::UserRole, tierName);
    item->setText(2, ToolBarManager::toolBarTierLabel(normalizedTier));

    auto* combo = qobject_cast<QComboBox*>(ui->toolbarTreeWidget->itemWidget(item, 2));
    if (!combo) {
        combo = new QComboBox(ui->toolbarTreeWidget);
        connect(
            combo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this, item, combo](int index) {
                if (index < 0) {
                    return;
                }

                setCustomToolbarTier(
                    item,
                    ToolBarManager::toolBarTierFromName(combo->currentData().toString())
                );
                exportCustomToolbars(selectedWorkbench().toLatin1());
                applyCustomToolbarTier(item);
            }
        );
        ui->toolbarTreeWidget->setItemWidget(item, 2, combo);
    }

    {
        QSignalBlocker block(combo);
        combo->clear();
        combo->addItem(
            ToolBarManager::toolBarTierLabel(ToolBarItem::Tier::Recommended),
            ToolBarManager::toolBarTierName(ToolBarItem::Tier::Recommended)
        );
        combo->addItem(
            ToolBarManager::toolBarTierLabel(ToolBarItem::Tier::Secondary),
            ToolBarManager::toolBarTierName(ToolBarItem::Tier::Secondary)
        );
        combo->addItem(
            ToolBarManager::toolBarTierLabel(ToolBarItem::Tier::Advanced),
            ToolBarManager::toolBarTierName(ToolBarItem::Tier::Advanced)
        );

        const int index = combo->findData(tierName);
        combo->setCurrentIndex(index >= 0 ? index : 1);
    }
}

void DlgCustomToolbars::applyCustomToolbarTier(QTreeWidgetItem* item)
{
    if (!item || item->parent() || type != Toolbar) {
        return;
    }

    const QString workbench = selectedWorkbench();
    if (workbench != QLatin1String("Global") && !isActiveWorkbenchSelection(workbench)) {
        return;
    }

    const auto persistenceKey = customToolbarPersistenceKey(item->text(0), workbench);
    if (persistenceKey.isEmpty()) {
        return;
    }

    for (auto* toolbar : getMainWindow()->findChildren<QToolBar*>()) {
        const auto toolbarKey = ToolBarManager::toolBarPersistenceKey(toolbar);
        if ((!toolbarKey.isEmpty() && toolbarKey == persistenceKey)
            || toolbar->objectName() == item->text(0)) {
            ToolBarManager::setToolBarPersistenceKey(toolbar, persistenceKey);
            ToolBarManager::setToolBarTier(toolbar, customToolbarTier(item));
            return;
        }
    }
}

void DlgCustomToolbars::updateToolbarItemMetadata(QTreeWidgetItem* item, const QString& workbench)
{
    if (!item) {
        return;
    }

    const auto persistenceKey = customToolbarPersistenceKey(item->text(0), workbench);
    item->setText(1, ToolBarManager::toolBarScopeLabel(persistenceKey));
    if (type == Toolbar) {
        setCustomToolbarTier(item, customToolbarTier(item));
    }
}

void DlgCustomToolbars::updateToolbarItemsMetadata()
{
    const QString workbench = selectedWorkbench();
    for (int i = 0; i < ui->toolbarTreeWidget->topLevelItemCount(); i++) {
        updateToolbarItemMetadata(ui->toolbarTreeWidget->topLevelItem(i), workbench);
    }
}

QString DlgCustomToolbars::findToolbarIdentityCollision(
    const QString& toolbarName,
    const QTreeWidgetItem* ignoredItem
) const
{
    const QString workbench = selectedWorkbench();
    for (int i = 0; i < ui->toolbarTreeWidget->topLevelItemCount(); i++) {
        const auto* item = ui->toolbarTreeWidget->topLevelItem(i);
        if (item == ignoredItem || item->text(0) != toolbarName) {
            continue;
        }

        return customToolbarPersistenceKey(item->text(0), workbench);
    }

    Workbench* sourceWorkbench = nullptr;
    if (workbench == QLatin1String("Global")) {
        sourceWorkbench = WorkbenchManager::instance()->active();
    }
    else {
        sourceWorkbench = WorkbenchManager::instance()->getWorkbench(workbench.toStdString());
    }

    if (!sourceWorkbench) {
        return {};
    }

    for (const auto& identity : sourceWorkbench->getToolbarIdentities()) {
        if (QString::fromUtf8(identity.first.c_str()) == toolbarName) {
            return QString::fromUtf8(identity.second.c_str());
        }
    }

    return {};
}

QString DlgCustomToolbars::toolbarIdentityCollisionMessage(
    const QString& toolbarName,
    const QString& persistenceKey
) const
{
    const auto scopeInfo = ToolBarManager::toolBarScopeInfo(persistenceKey);
    QString scopeLabel = ToolBarManager::toolBarScopeLabel(persistenceKey);
    if (scopeLabel.isEmpty()) {
        scopeLabel = tr("toolbar");
    }

    QString detail;
    if (!scopeInfo.workbench.isEmpty()) {
        QString menuText = Application::Instance->workbenchMenuText(scopeInfo.workbench);
        if (menuText == QLatin1String("<none>") || menuText.isEmpty()) {
            menuText = scopeInfo.workbench;
        }

        detail = tr(" in %1").arg(menuText);
    }

    return tr("The toolbar name '%1' collides with an existing %2 toolbar identity%3.")
        .arg(toolbarName, scopeLabel.toLower(), detail);
}

QString DlgCustomToolbars::selectedWorkbench() const
{
    return ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole).toString();
}

bool DlgCustomToolbars::isActiveWorkbenchSelection(const QString& workbench) const
{
    if (workbench == QLatin1String("Global")) {
        return false;
    }

    Workbench* activeWorkbench = WorkbenchManager::instance()->active();
    if (!activeWorkbench) {
        return false;
    }

    return workbench == QString::fromLatin1(activeWorkbench->name().c_str());
}

void DlgCustomToolbars::updateToolbarLayoutControls()
{
    const bool isToolbarDialog = type == Toolbar;
    ui->resetLayoutButton->setVisible(isToolbarDialog);
    ui->layoutScopeLabel->setVisible(isToolbarDialog);
    if (!isToolbarDialog) {
        return;
    }

    const QString workbench = selectedWorkbench();
    const bool activeSelection = isActiveWorkbenchSelection(workbench);
    auto* manager = ToolBarManager::getInstance();

    QString buttonText = tr("Reset Layout");
    QString scopeLabel;
    QString toolTip;
    bool enabled = false;

    if (workbench == QLatin1String("Global")) {
        scopeLabel = tr(
            "Layout scope: Global custom toolbars. Reset is only available for the active "
            "workbench layout."
        );
        toolTip = tr("Select the active workbench to reset its toolbar layout.");
    }
    else if (!activeSelection) {
        scopeLabel
            = tr("Layout scope: %1. Activate this workbench to reset its live toolbar layout.")
                  .arg(ui->workbenchBox->currentText());
        toolTip = tr("Reset is only available for the active workbench layout.");
    }
    else if (!manager) {
        scopeLabel = tr("Layout scope: Current workbench.");
    }
    else {
        const QString currentScope = manager->currentToolbarLayoutScopeLabel();
        const QString resetLabel = manager->currentToolbarLayoutResetLabel();

        scopeLabel = currentScope.isEmpty() ? tr("Layout scope: Current workbench") : currentScope;
        buttonText = resetLabel.isEmpty() ? tr("Reset Layout") : resetLabel;

        if (resetLabel.isEmpty()) {
            scopeLabel = tr("%1. Enable the per-workbench toolbar layout preference to reset it "
                            "here.")
                             .arg(scopeLabel);
            toolTip = tr(
                "Enable the per-workbench toolbar layout preference to reset the active "
                "toolbar layout."
            );
        }
        else {
            enabled = true;
            toolTip = resetLabel;
        }
    }

    ui->resetLayoutButton->setText(buttonText);
    ui->resetLayoutButton->setEnabled(enabled);
    ui->resetLayoutButton->setToolTip(toolTip);
    ui->layoutScopeLabel->setText(scopeLabel);
    ui->layoutScopeLabel->setToolTip(toolTip);
}

void DlgCustomToolbars::importCustomToolbars(const QByteArray& name)
{
    ParameterGrp::handle hGrp
        = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Workbench");
    const char* subgroup = (type == Toolbar ? "Toolbar" : "Toolboxbar");
    if (!hGrp->HasGroup(name.constData())) {
        return;
    }
    hGrp = hGrp->GetGroup(name.constData());
    if (!hGrp->HasGroup(subgroup)) {
        return;
    }
    hGrp = hGrp->GetGroup(subgroup);
    std::string separator = "Separator";

    std::vector<Base::Reference<ParameterGrp>> hGrps = hGrp->GetGroups();
    CommandManager& rMgr = Application::Instance->commandManager();
    for (const auto& hGrp : hGrps) {
        // create a toplevel item
        auto toplevel = new QTreeWidgetItem(ui->toolbarTreeWidget);
        bool active = hGrp->GetBool("Active", true);
        toplevel->setCheckState(0, (active ? Qt::Checked : Qt::Unchecked));
        if (type == Toolbar) {
            const auto tierName = QString::fromUtf8(hGrp->GetASCII("Tier").c_str());
            const auto tier = ToolBarManager::customToolBarTierFromName(tierName);
            toplevel->setData(2, Qt::UserRole, ToolBarManager::toolBarTierName(tier));
        }

        // get the elements of the subgroups
        std::vector<std::pair<std::string, std::string>> items = hGrp->GetASCIIMap();
        for (const auto& it2 : items) {
            // since we have stored the separators to the user parameters as (key, pair) we had to
            // make sure to use a unique key because otherwise we cannot store more than
            // one.
            if (it2.first.substr(0, separator.size()) == separator) {
                auto item = new QTreeWidgetItem(toplevel);
                item->setText(0, tr("<Separator>"));
                item->setData(0, Qt::UserRole, QByteArray("Separator"));
                item->setSizeHint(0, QSize(32, 32));
            }
            else if (it2.first == "Name") {
                QString toolbarName = QString::fromUtf8(it2.second.c_str());
                toplevel->setText(0, toolbarName);
            }
            else if (it2.first == "Tier") {
                continue;
            }
            else {
                Command* pCmd = rMgr.getCommandByName(it2.first.c_str());
                if (pCmd) {
                    // command name
                    auto* item = new QTreeWidgetItem(toplevel);
                    item->setText(0, Action::commandMenuText(pCmd));
                    item->setToolTip(0, Action::commandToolTip(pCmd));
                    item->setData(0, Qt::UserRole, QByteArray(it2.first.c_str()));
                    if (pCmd->getPixmap()) {
                        item->setIcon(0, BitmapFactory().iconFromTheme(pCmd->getPixmap()));
                    }
                    item->setSizeHint(0, QSize(32, 32));
                }
                else {
                    // If corresponding module is not yet loaded do not lose the entry
                    auto item = new QTreeWidgetItem(toplevel);
                    item->setText(0, tr("%1 module not loaded").arg(QString::fromStdString(it2.second)));
                    item->setData(0, Qt::UserRole, QByteArray(it2.first.c_str()));
                    item->setData(0, Qt::WhatsThisPropertyRole, QByteArray(it2.second.c_str()));
                    item->setSizeHint(0, QSize(32, 32));
                }
            }
        }

        updateToolbarItemMetadata(toplevel, QString::fromLatin1(name));
    }
}

void DlgCustomToolbars::exportCustomToolbars(const QByteArray& workbench)
{
    ParameterGrp::handle hGrp
        = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Workbench");
    const char* subgroup = (type == Toolbar ? "Toolbar" : "Toolboxbar");
    hGrp = hGrp->GetGroup(workbench.constData())->GetGroup(subgroup);
    hGrp->Clear();

    CommandManager& rMgr = Application::Instance->commandManager();
    for (int i = 0; i < ui->toolbarTreeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem* toplevel = ui->toolbarTreeWidget->topLevelItem(i);
        QString groupName = QStringLiteral("Custom_%1").arg(i + 1);
        QByteArray toolbarName = toplevel->text(0).toUtf8();
        ParameterGrp::handle hToolGrp = hGrp->GetGroup(groupName.toLatin1());
        hToolGrp->SetASCII("Name", toolbarName.constData());
        hToolGrp->SetBool("Active", toplevel->checkState(0) == Qt::Checked);
        if (type == Toolbar) {
            const auto tierName = ToolBarManager::toolBarTierName(customToolbarTier(toplevel));
            hToolGrp->SetASCII("Tier", tierName.toUtf8().constData());
        }

        // since we store the separators to the user parameters as (key, pair) we must
        // make sure to use a unique key because otherwise we cannot store more than
        // one.
        int suffixSeparator = 1;
        for (int j = 0; j < toplevel->childCount(); j++) {
            QTreeWidgetItem* child = toplevel->child(j);
            QByteArray commandName = child->data(0, Qt::UserRole).toByteArray();
            if (commandName == "Separator") {
                QByteArray key = commandName + QByteArray::number(suffixSeparator);
                suffixSeparator++;
                hToolGrp->SetASCII(key, commandName);
            }
            else {
                Command* pCmd = rMgr.getCommandByName(commandName);
                if (pCmd) {
                    hToolGrp->SetASCII(pCmd->getName(), pCmd->getAppModuleName());
                }
                else {
                    QByteArray moduleName = child->data(0, Qt::WhatsThisPropertyRole).toByteArray();
                    hToolGrp->SetASCII(commandName, moduleName);
                }
            }
        }
    }
}

/** Adds a new action */
void DlgCustomToolbars::onMoveActionRightButtonClicked()
{
    QTreeWidgetItem* item = ui->commandTreeWidget->currentItem();
    if (item) {
        if (!ui->toolbarTreeWidget->topLevelItemCount()) {
            onNewButtonClicked();
        }
        QTreeWidgetItem* current = ui->toolbarTreeWidget->currentItem();
        if (!current) {
            current = ui->toolbarTreeWidget->topLevelItem(0);
        }
        else if (current->parent()) {
            current = current->parent();
        }
        if (current && !current->parent()) {
            auto copy = new QTreeWidgetItem(current);
            copy->setText(0, item->text(1));
            copy->setIcon(0, item->icon(0));
            QByteArray data = item->data(1, Qt::UserRole).toByteArray();
            copy->setData(0, Qt::UserRole, data);
            copy->setSizeHint(0, QSize(32, 32));
            addCustomCommand(current->text(0), data);
        }
    }

    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());
}

/** Removes an action */
void DlgCustomToolbars::onMoveActionLeftButtonClicked()
{
    QTreeWidgetItem* item = ui->toolbarTreeWidget->currentItem();
    if (item && item->parent() && item->isSelected()) {
        QTreeWidgetItem* parent = item->parent();
        int index = parent->indexOfChild(item);
        parent->takeChild(index);

        // In case a separator should be moved we have to count the separators
        // which come before this one.
        // This is needed so that we can distinguish in removeCustomCommand
        // which separator it is.
        QByteArray data = item->data(0, Qt::UserRole).toByteArray();
        if (data == "Separator") {
            int countSep = 1;
            for (int i = 0; i < index - 1; i++) {
                QByteArray d = parent->child(i)->data(0, Qt::UserRole).toByteArray();
                if (d == "Separator") {
                    countSep++;
                }
            }

            data += QByteArray::number(countSep);
        }
        removeCustomCommand(parent->text(0), data);
        delete item;
    }

    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());
}

/** Moves up an action */
void DlgCustomToolbars::onMoveActionUpButtonClicked()
{
    QTreeWidgetItem* item = ui->toolbarTreeWidget->currentItem();
    if (item && item->parent() && item->isSelected()) {
        QTreeWidgetItem* parent = item->parent();
        int index = parent->indexOfChild(item);
        if (index > 0) {
            // In case a separator should be moved we have to count the separators
            // which come before this one.
            // This is needed so that we can distinguish in moveUpCustomCommand
            // which separator it is.
            QByteArray data = item->data(0, Qt::UserRole).toByteArray();
            if (data == "Separator") {
                int countSep = 1;
                for (int i = 0; i < index; i++) {
                    QByteArray d = parent->child(i)->data(0, Qt::UserRole).toByteArray();
                    if (d == "Separator") {
                        countSep++;
                    }
                }

                data += QByteArray::number(countSep);
            }

            parent->takeChild(index);
            parent->insertChild(index - 1, item);
            ui->toolbarTreeWidget->setCurrentItem(item);

            moveUpCustomCommand(parent->text(0), data);
        }
    }

    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());
}

/** Moves down an action */
void DlgCustomToolbars::onMoveActionDownButtonClicked()
{
    QTreeWidgetItem* item = ui->toolbarTreeWidget->currentItem();
    if (item && item->parent() && item->isSelected()) {
        QTreeWidgetItem* parent = item->parent();
        int index = parent->indexOfChild(item);
        if (index < parent->childCount() - 1) {
            // In case a separator should be moved we have to count the separators
            // which come before this one.
            // This is needed so that we can distinguish in moveDownCustomCommand
            // which separator it is.
            QByteArray data = item->data(0, Qt::UserRole).toByteArray();
            if (data == "Separator") {
                int countSep = 1;
                for (int i = 0; i < index; i++) {
                    QByteArray d = parent->child(i)->data(0, Qt::UserRole).toByteArray();
                    if (d == "Separator") {
                        countSep++;
                    }
                }

                data += QByteArray::number(countSep);
            }

            parent->takeChild(index);
            parent->insertChild(index + 1, item);
            ui->toolbarTreeWidget->setCurrentItem(item);

            moveDownCustomCommand(parent->text(0), data);
        }
    }

    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());
}

void DlgCustomToolbars::onNewButtonClicked()
{
    bool ok;
    QString text = QStringLiteral("Custom%1").arg(ui->toolbarTreeWidget->topLevelItemCount() + 1);
    text = QInputDialog::getText(
        this,
        tr("New toolbar"),
        tr("Toolbar name:"),
        QLineEdit::Normal,
        text,
        &ok,
        Qt::MSWindowsFixedSizeDialogHint
    );
    if (ok) {
        QString collision = findToolbarIdentityCollision(text);
        if (!collision.isEmpty()) {
            QMessageBox::warning(
                this,
                tr("Duplicated name"),
                toolbarIdentityCollisionMessage(text, collision)
            );
            return;
        }

        auto item = new QTreeWidgetItem(ui->toolbarTreeWidget);
        item->setText(0, text);
        updateToolbarItemMetadata(item, selectedWorkbench());
        item->setCheckState(0, Qt::Checked);
        item->setExpanded(true);

        QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
        QString workbench = data.toString();
        exportCustomToolbars(workbench.toLatin1());
        addCustomToolbar(text);
        applyCustomToolbarTier(item);
    }
}

void DlgCustomToolbars::onDeleteButtonClicked()
{
    QTreeWidgetItem* item = ui->toolbarTreeWidget->currentItem();
    if (item && !item->parent() && item->isSelected()) {
        int index = ui->toolbarTreeWidget->indexOfTopLevelItem(item);
        ui->toolbarTreeWidget->takeTopLevelItem(index);
        removeCustomToolbar(item->text(0));
        delete item;
    }

    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());
}

void DlgCustomToolbars::onResetLayoutButtonClicked()
{
    if (!ui->resetLayoutButton->isEnabled() || !isActiveWorkbenchSelection(selectedWorkbench())) {
        updateToolbarLayoutControls();
        return;
    }

    if (auto* manager = ToolBarManager::getInstance()) {
        manager->resetCurrentToolbarLayout();
    }

    updateToolbarLayoutControls();
}

void DlgCustomToolbars::onRenameButtonClicked()
{
    bool renamed = false;
    QTreeWidgetItem* item = ui->toolbarTreeWidget->currentItem();
    if (item && !item->parent() && item->isSelected()) {
        bool ok;
        QString old_text = item->text(0);
        QString text = QInputDialog::getText(
            this,
            tr("Rename toolbar"),
            tr("Toolbar name:"),
            QLineEdit::Normal,
            old_text,
            &ok,
            Qt::MSWindowsFixedSizeDialogHint
        );
        if (ok && text != old_text) {
            QString collision = findToolbarIdentityCollision(text, item);
            if (!collision.isEmpty()) {
                QMessageBox::warning(
                    this,
                    tr("Duplicated name"),
                    toolbarIdentityCollisionMessage(text, collision)
                );
                return;
            }

            item->setText(0, text);
            updateToolbarItemMetadata(item, selectedWorkbench());
            renameCustomToolbar(old_text, text);
            renamed = true;
            applyCustomToolbarTier(item);
        }
    }

    if (renamed) {
        QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
        QString workbench = data.toString();
        exportCustomToolbars(workbench.toLatin1());
    }
}

void DlgCustomToolbars::onAddMacroAction(const QByteArray&)
{}

void DlgCustomToolbars::onRemoveMacroAction(const QByteArray&)
{}

void DlgCustomToolbars::onModifyMacroAction(const QByteArray& macro)
{
    QVariant data = ui->categoryBox->itemData(ui->categoryBox->currentIndex(), Qt::UserRole);
    QString group = data.toString();
    if (group == QLatin1String("Macros")) {
        CommandManager& cCmdMgr = Application::Instance->commandManager();
        Command* pCmd = cCmdMgr.getCommandByName(macro);
        // the right side
        for (int i = 0; i < ui->toolbarTreeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* toplevel = ui->toolbarTreeWidget->topLevelItem(i);
            for (int j = 0; j < toplevel->childCount(); j++) {
                QTreeWidgetItem* item = toplevel->child(j);
                QByteArray command = item->data(0, Qt::UserRole).toByteArray();
                if (command == macro) {
                    item->setText(0, Action::commandMenuText(pCmd));
                    item->setToolTip(0, Action::commandToolTip(pCmd));
                    if (pCmd->getPixmap()) {
                        item->setIcon(0, BitmapFactory().iconFromTheme(pCmd->getPixmap()));
                    }
                }
            }
        }
        ui->categoryBox->activated(ui->categoryBox->currentIndex());
    }
}

void DlgCustomToolbars::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        updateToolbarTreeHeaders();
        int count = ui->categoryBox->count();

        CommandManager& cCmdMgr = Application::Instance->commandManager();
        for (int i = 0; i < count; i++) {
            QVariant data = ui->categoryBox->itemData(i, Qt::UserRole);
            std::vector<Command*> aCmds = cCmdMgr.getGroupCommands(data.toByteArray());
            if (!aCmds.empty()) {
                QString text = aCmds[0]->translatedGroupName();
                ui->categoryBox->setItemText(i, text);
            }
        }
        ui->categoryBox->activated(ui->categoryBox->currentIndex());
        updateToolbarItemsMetadata();
        updateToolbarLayoutControls();
    }
    else if (e->type() == QEvent::StyleChange) {
        ui->categoryBox->activated(ui->categoryBox->currentIndex());
    }

    QWidget::changeEvent(e);
}

// -------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::DlgCustomToolbarsImp */

/**
 *  Constructs a DlgCustomToolbarsImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgCustomToolbarsImp::DlgCustomToolbarsImp(QWidget* parent)
    : DlgCustomToolbars(DlgCustomToolbars::Toolbar, parent)
{}

/** Destroys the object and frees any allocated resources */
DlgCustomToolbarsImp::~DlgCustomToolbarsImp() = default;

void DlgCustomToolbarsImp::addCustomToolbar(const QString& name)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QToolBar* bar = getMainWindow()->addToolBar(name);
        bar->setObjectName(name);
    }
}

void DlgCustomToolbarsImp::removeCustomToolbar(const QString& name)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1) {
            return;
        }

        QToolBar* tb = bars.front();
        getMainWindow()->removeToolBar(tb);
        delete tb;
    }
}

void DlgCustomToolbarsImp::renameCustomToolbar(const QString& old_name, const QString& new_name)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(old_name);
        if (bars.size() != 1) {
            return;
        }

        QToolBar* tb = bars.front();
        tb->setObjectName(new_name);
        tb->setWindowTitle(new_name);
    }
}

QList<QAction*> DlgCustomToolbarsImp::getActionGroup(QAction* action)
{
    QList<QAction*> group;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QList<QWidget*> widgets = action->associatedWidgets();
#else
    QList<QObject*> widgets = action->associatedObjects();
#endif
    for (const auto& widget : widgets) {
        auto tb = qobject_cast<QToolButton*>(widget);
        if (tb) {
            QMenu* menu = tb->menu();
            if (menu) {
                group = menu->actions();
                break;
            }
        }
    }
    return group;
}

void DlgCustomToolbarsImp::setActionGroup(QAction* action, const QList<QAction*>& group)
{
    // See also ActionGroup::addTo()
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QList<QWidget*> widgets = action->associatedWidgets();
#else
    QList<QObject*> widgets = action->associatedObjects();
#endif
    for (const auto& widget : widgets) {
        auto tb = qobject_cast<QToolButton*>(widget);
        if (tb) {
            QMenu* menu = tb->menu();
            if (!menu) {
                tb->setPopupMode(QToolButton::MenuButtonPopup);
                tb->setObjectName(QStringLiteral("qt_toolbutton_menubutton"));
                auto menu = new QMenu(tb);
                menu->addActions(group);
                tb->setMenu(menu);
            }
        }
    }
}

void DlgCustomToolbarsImp::addCustomCommand(const QString& name, const QByteArray& cmd)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1) {
            return;
        }

        if (cmd == "Separator") {
            QAction* action = bars.front()->addSeparator();
            action->setData(QByteArray("Separator"));
        }
        else {
            CommandManager& mgr = Application::Instance->commandManager();
            if (mgr.addTo(cmd, bars.front())) {
                QAction* action = bars.front()->actions().last();
                // See ToolBarManager::setup(ToolBarItem* , QToolBar* )
                // We have to add the user data in order to identify the command in
                // removeCustomCommand(), moveUpCustomCommand() or moveDownCustomCommand()
                if (action && action->data().isNull()) {
                    action->setData(cmd);
                }
            }
        }
    }
}

void DlgCustomToolbarsImp::removeCustomCommand(const QString& name, const QByteArray& userdata)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1) {
            return;
        }

        QByteArray cmd = userdata;
        int numSep = 0, indexSep = 0;
        if (cmd.startsWith("Separator")) {
            numSep = cmd.mid(9).toInt();
            cmd = "Separator";
        }
        QList<QAction*> actions = bars.front()->actions();
        for (const auto& action : actions) {
            if (action->data().toByteArray() == cmd) {
                // if we move a separator then make sure to pick up the right one
                if (numSep > 0) {
                    if (++indexSep < numSep) {
                        continue;
                    }
                }
                bars.front()->removeAction(action);
                break;
            }
        }
    }
}

void DlgCustomToolbarsImp::moveUpCustomCommand(const QString& name, const QByteArray& userdata)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1) {
            return;
        }

        QByteArray cmd = userdata;
        int numSep = 0, indexSep = 0;
        if (cmd.startsWith("Separator")) {
            numSep = cmd.mid(9).toInt();
            cmd = "Separator";
        }
        QList<QAction*> actions = bars.front()->actions();
        QAction* before = nullptr;
        for (const auto& action : actions) {
            if (action->data().toByteArray() == cmd) {
                // if we move a separator then make sure to pick up the right one
                if (numSep > 0) {
                    if (++indexSep < numSep) {
                        before = action;
                        continue;
                    }
                }
                if (before) {
                    QList<QAction*> group = getActionGroup(action);
                    bars.front()->removeAction(action);
                    bars.front()->insertAction(before, action);
                    if (!group.isEmpty()) {
                        setActionGroup(action, group);
                    }
                    break;
                }
            }

            before = action;
        }
    }
}

void DlgCustomToolbarsImp::moveDownCustomCommand(const QString& name, const QByteArray& userdata)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1) {
            return;
        }

        QByteArray cmd = userdata;
        int numSep = 0, indexSep = 0;
        if (cmd.startsWith("Separator")) {
            numSep = cmd.mid(9).toInt();
            cmd = "Separator";
        }
        QList<QAction*> actions = bars.front()->actions();
        for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
            if ((*it)->data().toByteArray() == cmd) {
                // if we move a separator then make sure to pick up the right one
                if (numSep > 0) {
                    if (++indexSep < numSep) {
                        continue;
                    }
                }
                QAction* act = *it;
                if (*it == actions.back()) {
                    break;  // we're already on the last element
                }
                ++it;
                // second last item
                if (*it == actions.back()) {
                    QList<QAction*> group = getActionGroup(act);
                    bars.front()->removeAction(act);
                    bars.front()->addAction(act);
                    if (!group.isEmpty()) {
                        setActionGroup(act, group);
                    }
                    break;
                }
                ++it;
                QList<QAction*> group = getActionGroup(act);
                bars.front()->removeAction(act);
                bars.front()->insertAction(*it, act);
                if (!group.isEmpty()) {
                    setActionGroup(act, group);
                }
                break;
            }
        }
    }
}

void DlgCustomToolbarsImp::showEvent(QShowEvent* event)
{
    // If we did this already in the constructor we wouldn't get the vertical scrollbar if needed.
    // The problem was noticed with Qt 4.1.4 but may arise with any later version.
    if (firstShow) {
        ui->categoryBox->activated(ui->categoryBox->currentIndex());
        firstShow = false;
    }

    DlgCustomToolbars::showEvent(event);
}

void DlgCustomToolbarsImp::changeEvent(QEvent* e)
{
    DlgCustomToolbars::changeEvent(e);
}


/* TRANSLATOR Gui::Dialog::DlgCustomToolBoxbarsImp */

/**
 *  Constructs a DlgCustomToolBoxbarsImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgCustomToolBoxbarsImp::DlgCustomToolBoxbarsImp(QWidget* parent)
    : DlgCustomToolbars(DlgCustomToolbars::Toolboxbar, parent)
{
    setWindowTitle(tr("Toolbox Bars"));
}

/** Destroys the object and frees any allocated resources */
DlgCustomToolBoxbarsImp::~DlgCustomToolBoxbarsImp() = default;

void DlgCustomToolBoxbarsImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        setWindowTitle(tr("Toolbox Bars"));
    }
    DlgCustomToolbars::changeEvent(e);
}

#include "moc_DlgToolbarsImp.cpp"
