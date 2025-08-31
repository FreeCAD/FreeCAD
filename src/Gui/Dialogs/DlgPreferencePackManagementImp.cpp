/***************************************************************************
 *   Copyright (c) 2022 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
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
# include <QMessageBox>
#endif

#include "Dialogs/DlgPreferencePackManagementImp.h"
#include "ui_DlgPreferencePackManagement.h"
#include "Application.h"
#include "Command.h"
#include "PreferencePackManager.h"


using namespace Gui::Dialog;
namespace fs = std::filesystem;

/* TRANSLATOR Gui::Dialog::DlgPreferencePackManagementImp */

/**
 *  Constructs a Gui::Dialog::DlgPreferencePackManagementImp as a child of 'parent'
 */
DlgPreferencePackManagementImp::DlgPreferencePackManagementImp(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_DlgPreferencePackManagement)
{
    ui->setupUi(this);
    connect(ui->pushButtonOpenAddonManager, &QPushButton::clicked, this, &DlgPreferencePackManagementImp::showAddonManager);
    connect(this, &DlgPreferencePackManagementImp::packVisibilityChanged, this, &DlgPreferencePackManagementImp::updateTree);
    updateTree();
}

void DlgPreferencePackManagementImp::updateTree()
{
    // Separate out user-saved packs from installed packs: we can remove individual user-saved packs,
    // but can only disable individual installed packs (though we can completely uninstall the pack's
    // containing Addon by redirecting to the Addon Manager).
    auto savedPreferencePacksDirectory = Application::Instance->prefPackManager()->getSavedPreferencePacksPath();
    auto modDirectories = Application::Instance->prefPackManager()->modPaths();
    auto resourcePath = Application::Instance->prefPackManager()->getResourcePreferencePacksPath();

    // The displayed tree has two levels: at the toplevel is either "User-Saved Packs" or the name
    // of the addon containing the pack. Beneath those are the individual packs themselves. The tree view shows
    // "Hide"/"Show" for packs installed as a Mod, and "Delete" for packs in the user-saved pack
    // section.
    auto userPacks = Application::Instance->prefPackManager()->getPacksFromDirectory(savedPreferencePacksDirectory);

    auto builtinPacks = Application::Instance->prefPackManager()->getPacksFromDirectory(resourcePath);

    std::map<std::string, std::vector<std::string>> installedPacks;
    for (const auto& modDirectory : modDirectories) {
        if (fs::exists(modDirectory) && fs::is_directory(modDirectory)) {
            for (const auto& mod : fs::directory_iterator(modDirectory)) {
                auto packs = Application::Instance->prefPackManager()->getPacksFromDirectory(mod);
                if (!packs.empty()) {
                    auto modName = Base::FileInfo::pathToString(mod.path().filename());
                    installedPacks.emplace(modName, packs);
                }
            }
        }
    }

    ui->treeWidget->clear(); // Begin by clearing whatever is there
    ui->treeWidget->header()->setDefaultAlignment(Qt::AlignLeft);
    ui->treeWidget->setColumnCount(2);
    ui->treeWidget->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    ui->treeWidget->header()->setStretchLastSection(false);
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);

    if (!userPacks.empty()) {
        addTreeNode(tr("User-Saved Preference Packs").toStdString(), userPacks, TreeWidgetType::USER);
    }

    if (!builtinPacks.empty()) {
        addTreeNode(tr("Built-In Preference Packs").toStdString(), builtinPacks, TreeWidgetType::BUILTIN);
    }

    for (const auto& installedPack : installedPacks) {
        addTreeNode(installedPack.first, installedPack.second, TreeWidgetType::ADDON);
    }
}

void DlgPreferencePackManagementImp::addTreeNode(const std::string &name, const std::vector<std::string> &contents, TreeWidgetType twt)
{
    static const auto iconIsVisible = QIcon(QLatin1String(":/icons/dagViewVisible.svg"));
    static const auto iconIsInvisible = QIcon(QLatin1String(":/icons/Invisible.svg"));
    auto packRoot = new QTreeWidgetItem();
    packRoot->setText(0, QString::fromStdString(name));
    std::vector<QTreeWidgetItem*> items;
    for (const auto& packName : contents) {
        auto pack = new QTreeWidgetItem(packRoot);
        pack->setText(0, QString::fromStdString(packName));
        items.push_back(pack);
    }
    ui->treeWidget->addTopLevelItem(packRoot);
    packRoot->setExpanded(true);
    for (const auto item : items) {
        auto button = new QPushButton();
        button->setFlat(true);
        switch (twt) {
        break; case TreeWidgetType::BUILTIN:
            // The button is a "hide" button
            if (Application::Instance->prefPackManager()->isVisible("##BUILT_IN##", item->text(0).toStdString()))
                button->setIcon(iconIsVisible);
            else
                button->setIcon(iconIsInvisible);
            button->setToolTip(tr("Toggle visibility of built-in preference pack '%1'").arg(item->text(0)));
            connect(button, &QPushButton::clicked, [this, item]() {
                this->hideBuiltInPack(item->text(0).toStdString());
                });
        break; case TreeWidgetType::USER:
            // The button is a "delete" button
            button->setIcon(QIcon(QLatin1String(":/icons/delete.svg")));
            button->setToolTip(tr("Deletes the user-saved preference pack '%1'").arg(item->text(0)));
            connect(button, &QPushButton::clicked, [this, item]() {
                this->deleteUserPack(item->text(0).toStdString());
                });
        break; case TreeWidgetType::ADDON:
            // The button is a "hide" button
            if (Application::Instance->prefPackManager()->isVisible(name, item->text(0).toStdString()))
                button->setIcon(iconIsVisible);
            else
                button->setIcon(iconIsInvisible);
            button->setToolTip(tr("Toggles the visibility of the addon preference pack '%1' (use the Addon Manager to remove permanently)").arg(item->text(0)));
            connect(button, &QPushButton::clicked, [this, name, item]() {
                this->hideInstalledPack(name, item->text(0).toStdString());
                });
        }
        ui->treeWidget->setItemWidget(item, 1, button);
    }
}

void DlgPreferencePackManagementImp::deleteUserPack(const std::string& name)
{
    // Do the deletion here...
    auto result = QMessageBox::warning(this, tr("Delete saved preference pack?"),
        tr("Delete the preference pack named '%1'? This cannot be undone.").arg(QString::fromStdString(name)),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (result == QMessageBox::Yes) {
        Application::Instance->prefPackManager()->deleteUserPack(name);
        Q_EMIT packVisibilityChanged();
    }
}

void DlgPreferencePackManagementImp::hideBuiltInPack(const std::string& prefPackName)
{
    Application::Instance->prefPackManager()->toggleVisibility("##BUILT_IN##", prefPackName);
    Q_EMIT packVisibilityChanged();
}

void DlgPreferencePackManagementImp::hideInstalledPack(const std::string& addonName, const std::string& prefPackName)
{
    Application::Instance->prefPackManager()->toggleVisibility(addonName, prefPackName);
    Q_EMIT packVisibilityChanged();
}

void DlgPreferencePackManagementImp::showAddonManager()
{
    // Configure the view to show all preference packs (installed and uninstalled)
    auto pref = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Addons");
    pref->SetInt("PackageTypeSelection", 3);
    pref->SetInt("StatusSelection", 0);

    CommandManager& rMgr = Application::Instance->commandManager();
    rMgr.runCommandByName("Std_AddonMgr");
    close();
}

DlgPreferencePackManagementImp::~DlgPreferencePackManagementImp() = default;



#include "moc_DlgPreferencePackManagementImp.cpp"
