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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QPushButton>
#endif

#include <Gui/Application.h>
#include <Gui/ParamHandler.h>

#include "DlgSettingsUI.h"
#include "ui_DlgSettingsUI.h"

#include "Dialogs/DlgThemeEditor.h"

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
    ui->iconSizeSpinBox->onSave();
    ui->rowSpacingSpinBox->onSave();
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
}

void DlgSettingsUI::loadSettings()
{
    // Theme
    ui->ThemeAccentColor1->onRestore();
    ui->ThemeAccentColor2->onRestore();
    ui->ThemeAccentColor3->onRestore();

    // Tree View
    ui->iconSizeSpinBox->onRestore();
    ui->rowSpacingSpinBox->onRestore();
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

    loadStyleSheet();
}

void DlgSettingsUI::loadStyleSheet()
{
    populateStylesheets("StyleSheet", "qss", ui->StyleSheets, "No style sheet");
    populateStylesheets("OverlayActiveStyleSheet", "overlay", ui->OverlayStyleSheets, "Auto");
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

namespace {

void applyStyleSheet(ParameterGrp *hGrp)
{
    if (auto parameterManager = Base::provideService<Gui::StyleParameters::ParameterManager>()) {
        parameterManager->reload();
    }

    auto sheet = hGrp->GetASCII("StyleSheet");
    bool tiledBG = hGrp->GetBool("TiledBackground", false);
    Gui::Application::Instance->setStyleSheet(QString::fromUtf8(sheet.c_str()), tiledBG);
}

} // anonymous namespace

void DlgSettingsUI::attachObserver()
{
    static ParamHandlers handlers;

    auto handler = handlers.addDelayedHandler("BaseApp/Preferences/MainWindow",
                               {"StyleSheet", "TiledBackground"},
                               applyStyleSheet);
    handlers.addHandler("BaseApp/Preferences/Themes",
                        {"ThemeAccentColor1", "ThemeAccentColor2", "ThemeAccentColor2"},
                        handler);
}

#include "moc_DlgSettingsUI.cpp"

