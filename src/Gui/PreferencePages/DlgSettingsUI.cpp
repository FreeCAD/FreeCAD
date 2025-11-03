/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <filesystem>
#include <vector>

# include <QPushButton>

#include <Gui/Application.h>
#include <Gui/ParamHandler.h>

#include "DlgSettingsUI.h"
#include "ui_DlgSettingsUI.h"

#include "Dialogs/DlgThemeEditor.h"

#include <Base/Console.h>
#include <Base/ServiceProvider.h>


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsUI */

/**
 *  Constructs a DlgSettingsUI which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsUI::DlgSettingsUI(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsUI)
{
    ui->setupUi(this);

    connect(ui->themeEditorButton, &QPushButton::clicked, [this]() {
        openThemeEditor();
    });
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsUI::~DlgSettingsUI() = default;

void DlgSettingsUI::saveSettings()
{
    // Theme
    ui->ThemeAccentColor1->onSave();
    ui->ThemeAccentColor2->onSave();
    ui->ThemeAccentColor3->onSave();
    ui->StyleSheets->onSave();
    ui->OverlayStyleSheets->onSave();

    // Tree View
    ui->fontSizeSpinBox->onSave();
    ui->iconSizeSpinBox->onSave();
    ui->resizableColumnsCheckBox->onSave();
    ui->showVisibilityIconCheckBox->onSave();
    ui->hideDescriptionCheckBox->onSave();
    ui->hideInternalNamesCheckBox->onSave();
    ui->hideTreeViewScrollBarCheckBox->onSave();
    ui->hideHeaderCheckBox->onSave();

    // Overlay
    ui->hideTabBarCheckBox->onSave();
    ui->hintShowTabBarCheckBox->onSave();
    ui->hidePropertyViewScrollBarCheckBox->onSave();
    ui->overlayAutoHideCheckBox->onSave();
    ui->mouseClickPassThroughCheckBox->onSave();
    ui->mouseWheelPassThroughCheckBox->onSave();

    // TaskWatcher
    ui->showTaskWatcherCheckBox->onSave();
}

void DlgSettingsUI::loadSettings()
{
    // Theme
    ui->ThemeAccentColor1->onRestore();
    ui->ThemeAccentColor2->onRestore();
    ui->ThemeAccentColor3->onRestore();

    // Tree View
    ui->fontSizeSpinBox->onRestore();
    ui->iconSizeSpinBox->onRestore();
    ui->resizableColumnsCheckBox->onRestore();
    ui->showVisibilityIconCheckBox->onRestore();
    ui->hideDescriptionCheckBox->onRestore();
    ui->hideInternalNamesCheckBox->onRestore();
    ui->hideTreeViewScrollBarCheckBox->onRestore();
    ui->hideHeaderCheckBox->onRestore();

    // Overlay
    ui->hideTabBarCheckBox->onRestore();
    ui->hintShowTabBarCheckBox->onRestore();
    ui->hidePropertyViewScrollBarCheckBox->onRestore();
    ui->overlayAutoHideCheckBox->onRestore();
    ui->mouseClickPassThroughCheckBox->onRestore();
    ui->mouseWheelPassThroughCheckBox->onRestore();

    // TaskWatcher
    ui->showTaskWatcherCheckBox->onRestore();

    loadStyleSheet();
}

void DlgSettingsUI::loadStyleSheet()
{
    populateStylesheets("StyleSheet", "qss", ui->StyleSheets, "No style sheet");
    populateStylesheets("OverlayActiveStyleSheet", "overlay", ui->OverlayStyleSheets, "Auto");
}

void DlgSettingsUI::loadThemeDefaults()
{
    // get current theme name
    ParameterGrp::handle hMainWindow = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow");
    std::string themeName = hMainWindow->GetASCII("Theme", "");

    if (themeName.empty()) {
#ifdef FC_DEBUG
        Base::Console().message("No theme set, skipping theme color defaults\n");
#endif
        return;  // no theme active, use current colors
    }

#ifdef FC_DEBUG
    Base::Console().message("Loading UI theme color defaults for theme: %s\n", themeName.c_str());
#endif

    // construct path to the theme's config file
    std::string appPath = App::Application::getResourceDir();
    std::filesystem::path configFile = std::filesystem::path(appPath) / "Gui" / "PreferencePacks"
        / themeName / (themeName + ".cfg");

    if (!std::filesystem::exists(configFile)) {
#ifdef FC_DEBUG
        Base::Console().warning("Config file not found for theme '%s'\n", themeName.c_str());
#endif
        return;
    }

    // load theme config into temporary parameters
    auto themeParams = ParameterManager::Create();
    themeParams->LoadDocument(Base::FileInfo::pathToString(configFile).c_str());

    // try to get the Themes group from the theme config
    // NOTE: not all themes may have accent colors
    try {
        auto themeThemesGroup =
            themeParams->GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Themes");

        // get the current user's Themes group
        auto userThemesGroup =
            App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Themes");

        // list of theme accent color parameters to copy from theme
        std::vector<std::string> accentColors = {"ThemeAccentColor1",
                                                  "ThemeAccentColor2",
                                                  "ThemeAccentColor3"};

        // copy only the accent colors from theme to user config
        for (const auto& colorName : accentColors) {
            unsigned long colorValue = themeThemesGroup->GetUnsigned(colorName.c_str(), UINT_MAX);
            if (colorValue != UINT_MAX) {
                userThemesGroup->SetUnsigned(colorName.c_str(), colorValue);
            }
        }
    }
    catch (...) {
#ifdef FC_DEBUG
        Base::Console().message("Theme '%s' does not define accent colors, using defaults\n",
                                themeName.c_str());
#endif
    }
    try {
        auto themeMainWindowGroup =
            themeParams->GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("MainWindow");

        // get the current user's MainWindow group
        auto userMainWindowGroup =
            App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");

        // Load StyleSheet parameter
        std::string styleSheet = themeMainWindowGroup->GetASCII("StyleSheet", "");
        if (!styleSheet.empty()) {
            userMainWindowGroup->SetASCII("StyleSheet", styleSheet.c_str());
        }

        // Load OverlayActiveStyleSheet parameter
        std::string overlayStyleSheet = themeMainWindowGroup->GetASCII("OverlayActiveStyleSheet", "");
        if (!overlayStyleSheet.empty()) {
            userMainWindowGroup->SetASCII("OverlayActiveStyleSheet", overlayStyleSheet.c_str());
        }
    }
    catch (...) {
#ifdef FC_DEBUG
        Base::Console().message("Theme '%s' does not define stylesheet settings, using defaults\n",
                themeName.c_str());
#endif
    }
}

void DlgSettingsUI::resetSettingsToDefaults()
{
    // get parameter group
    ParameterGrp::handle hThemes =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Themes");

    // remove all theme accent color parameters so theme defaults can be applied fresh
    std::vector<std::string> accentColors = {"ThemeAccentColor1",
                                              "ThemeAccentColor2",
                                              "ThemeAccentColor3"};

    for (const auto& colorName : accentColors) {
        hThemes->RemoveUnsigned(colorName.c_str());
    }

    // reset all the parameters associated to Gui::Pref* widgets
    PreferencePage::resetSettingsToDefaults();

    // apply theme specific color defaults
    loadThemeDefaults();

    // reload the settings to update the GUI
    loadSettings();
}

void DlgSettingsUI::populateStylesheets(const char* key,
                                        const char* path,
                                        PrefComboBox* combo,
                                        const char* def,
                                        QStringList filter)
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow");
    // List all .qss/.css files
    QMap<QString, QString> cssFiles;
    QDir dir;
    if (filter.isEmpty()) {
        filter << QStringLiteral("*.qss");
        filter << QStringLiteral("*.css");
    }
    QFileInfoList fileNames;

    // read from user, resource and built-in directory
    QStringList qssPaths = QDir::searchPaths(QString::fromUtf8(path));
    for (QStringList::iterator it = qssPaths.begin(); it != qssPaths.end(); ++it) {
        dir.setPath(*it);
        fileNames = dir.entryInfoList(filter, QDir::Files, QDir::Name);
        for (QFileInfoList::iterator jt = fileNames.begin(); jt != fileNames.end(); ++jt) {
            if (cssFiles.find(jt->baseName()) == cssFiles.end()) {
                cssFiles[jt->baseName()] = jt->fileName();
            }
        }
    }

    combo->clear();

    // now add all unique items
    combo->addItem(tr(def), QStringLiteral(""));
    for (QMap<QString, QString>::iterator it = cssFiles.begin(); it != cssFiles.end(); ++it) {
        combo->addItem(it.key(), it.value());
    }

    QString selectedStyleSheet = QString::fromUtf8(hGrp->GetASCII(key).c_str());
    int index = combo->findData(selectedStyleSheet);

    // might be an absolute path name
    if (index < 0 && !selectedStyleSheet.isEmpty()) {
        QFileInfo fi(selectedStyleSheet);
        if (fi.isAbsolute()) {
            QString path = fi.absolutePath();
            if (qssPaths.indexOf(path) >= 0) {
                selectedStyleSheet = fi.fileName();
            }
            else {
                selectedStyleSheet = fi.absoluteFilePath();
                combo->addItem(fi.baseName(), selectedStyleSheet);
            }
        }
    }

    combo->setCurrentIndex(index);
    combo->onRestore();
}

void DlgSettingsUI::openThemeEditor()
{
    Gui::DlgThemeEditor editor;
    editor.exec();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsUI::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        loadStyleSheet();
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsUI.cpp"

