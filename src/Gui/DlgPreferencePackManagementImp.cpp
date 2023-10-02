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

#include "DlgPreferencePackManagementImp.h"
#include "ui_DlgPreferencePackManagement.h"
#include "Application.h"
#include "Command.h"
#include "PreferencePackManager.h"


using namespace Gui::Dialog;
namespace fs = boost::filesystem;

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
}

void DlgPreferencePackManagementImp::showEvent(QShowEvent* event)
{
    // Separate out user-saved packs from installed packs: we can remove individual user-saved packs,
    // but can only disable individual installed packs (though we can completely uninstall the pack's
    // containing Addon by redirecting to the Addon Manager).
    auto savedPreferencePacksDirectory = fs::path(App::Application::getUserAppDataDir()) / "SavedPreferencePacks";
    auto modDirectory = fs::path(App::Application::getUserAppDataDir()) / "Mod";
    auto resourcePath = fs::path(App::Application::getResourceDir()) / "Gui" / "PreferencePacks";

    // The displayed tree has two levels: at the toplevel is either "User-Saved Packs" or the name
    // of the addon containing the pack. Beneath those are the individual packs themselves. The tree view shows
    // "Hide"/"Show" for packs installed as a Mod, and "Delete" for packs in the user-saved pack
    // section.
    auto userPacks = getPacksFromDirectory(savedPreferencePacksDirectory);

    auto builtinPacks = getPacksFromDirectory(resourcePath);

    std::map<std::string, std::vector<std::string>> installedPacks;
    if (fs::exists(modDirectory) && fs::is_directory(modDirectory)) {
        for (const auto& mod : fs::directory_iterator(modDirectory)) {
            auto packs = getPacksFromDirectory(mod);
            if (!packs.empty()) {
                auto modName = mod.path().filename().string();
                installedPacks.emplace(modName, packs);
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

    if (event)
        QDialog::showEvent(event);
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
            connect(button, &QPushButton::clicked, [this, name, item]() {
                this->hideBuiltInPack(item->text(0).toStdString());
                });
        break; case TreeWidgetType::USER:
            // The button is a "delete" button
            button->setIcon(QIcon(QLatin1String(":/icons/delete.svg")));
            button->setToolTip(tr("Delete user-saved preference pack '%1'").arg(item->text(0)));
            connect(button, &QPushButton::clicked, [this, item]() {
                this->deleteUserPack(item->text(0).toStdString());
                });
        break; case TreeWidgetType::ADDON:
            // The button is a "hide" button
            if (Application::Instance->prefPackManager()->isVisible(name, item->text(0).toStdString()))
                button->setIcon(iconIsVisible);
            else
                button->setIcon(iconIsInvisible);
            button->setToolTip(tr("Toggle visibility of Addon preference pack '%1' (use Addon Manager to permanently remove)").arg(item->text(0)));
            connect(button, &QPushButton::clicked, [this, name, item]() {
                this->hideInstalledPack(name, item->text(0).toStdString());
                });
        }
        ui->treeWidget->setItemWidget(item, 1, button);
    }
}

std::vector<std::string> DlgPreferencePackManagementImp::getPacksFromDirectory(const fs::path& path) const
{
    std::vector<std::string> results;
    auto packageMetadataFile = path / "package.xml";
    if (fs::exists(packageMetadataFile) && fs::is_regular_file(packageMetadataFile)) {
        try {
            App::Metadata metadata(packageMetadataFile);
            auto content = metadata.content();
            for (const auto& item : content) {
                if (item.first == "preferencepack") {
                    results.push_back(item.second.name());
                }
            }
        }
        catch (...) {
            // Failed to read the metadata, or to create the preferencePack based on it...
            Base::Console().Error(("Failed to read " + packageMetadataFile.string()).c_str());
        }
    }
    return results;
}


void DlgPreferencePackManagementImp::deleteUserPack(const std::string& name)
{
    // Do the deletion here...
    auto result = QMessageBox::warning(this, tr("Delete saved preference pack?"),
        tr("Are you sure you want to delete the preference pack named '%1'? This cannot be undone.").arg(QString::fromStdString(name)),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (result == QMessageBox::Yes) {
        Application::Instance->prefPackManager()->deleteUserPack(name);
        showEvent(nullptr);
        Q_EMIT packVisibilityChanged();
    }
}

void DlgPreferencePackManagementImp::hideBuiltInPack(const std::string& prefPackName)
{
    Application::Instance->prefPackManager()->toggleVisibility("##BUILT_IN##", prefPackName);
    showEvent(nullptr);
    Q_EMIT packVisibilityChanged();
}

void DlgPreferencePackManagementImp::hideInstalledPack(const std::string& addonName, const std::string& prefPackName)
{
    Application::Instance->prefPackManager()->toggleVisibility(addonName, prefPackName);
    showEvent(nullptr);
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
