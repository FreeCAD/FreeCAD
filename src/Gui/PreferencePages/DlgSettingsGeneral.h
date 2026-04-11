// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2023 FreeCAD Project Association                         *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#pragma once

#include <Gui/PropertyPage.h>
#include <memory>
#include <string>

class QTabWidget;

namespace Gui
{
namespace Dialog
{
class Ui_DlgSettingsGeneral;
class DlgCreateNewPreferencePackImp;
class DlgPreferencePackManagementImp;
class DlgRevertToBackupConfigImp;

/** This class implements the settings for the application.
 *  You can change window style, size of pixmaps, size of recent file list and so on
 *  \author Werner Mayer
 */
class DlgSettingsGeneral: public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsGeneral(QWidget* parent = nullptr);
    ~DlgSettingsGeneral() override;

    void saveSettings() override;
    void loadSettings() override;
    void resetSettingsToDefaults() override;

    void saveThemes();
    void loadThemes();

    static void attachObserver();

protected:
    void changeEvent(QEvent* event) override;

protected Q_SLOTS:
    void onLoadPreferencePackClicked(const std::string& packName);
    void recreatePreferencePackMenu();
    void newPreferencePackDialogAccepted();
    void onManagePreferencePacksClicked();
    void onImportConfigClicked();
    void onThemeChanged(int index);
    void onLinkActivated(const QString& link);

public Q_SLOTS:
    void onUnitSystemIndexChanged(int index);

private:
    void saveUnitSystemSettings();
    void saveDockWindowVisibility();
    void loadDockWindowVisibility();
    void setRecentFileSize();
    void saveAsNewPreferencePack();
    void revertToSavedConfig();
    bool setLanguage();  // Returns true if language has been changed
    void setNumberLocale(bool force = false);
    void setDecimalPointConversion(bool on);
    void retranslateUnits();
    int getCurrentIconSize() const;
    void addIconSizes(int current);
    void translateIconSizes();

private:
    int localeIndex;
    bool themeChanged;
    std::unique_ptr<Ui_DlgSettingsGeneral> ui;
    std::unique_ptr<DlgCreateNewPreferencePackImp> newPreferencePackDialog;
    std::unique_ptr<DlgPreferencePackManagementImp> preferencePackManagementDialog;
    std::unique_ptr<DlgRevertToBackupConfigImp> revertToBackupConfigDialog;
};

}  // namespace Dialog
}  // namespace Gui
