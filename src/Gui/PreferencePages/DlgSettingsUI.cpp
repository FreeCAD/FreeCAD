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
// Qt headers
#include <QColor>
#include <QApplication>
// Base color packing helpers
#include <Base/Color.h>
// platform headers for system accent color
#ifdef FC_OS_WIN32
#  include <windows.h>
#  include <dwmapi.h>
#  pragma comment(lib, "dwmapi.lib")
#endif
#ifdef FC_OS_MACOSX
#  include <CoreFoundation/CoreFoundation.h>
#endif


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsUI */

namespace {
QColor getSystemAccentColor();
}

/**
 *  Constructs a DlgSettingsUI which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsUI::DlgSettingsUI(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsUI)
{
    ui->setupUi(this);

    connect(ui->themeEditorButton, &QPushButton::clicked, [this]() { openThemeEditor(); });

    // helper: update preview color and enable/disable pickers
    auto updateAccentUi = [this]() {
        QColor accent;
        bool useSystem = false;

        if (ui->UseSystemAccentColors) {
            useSystem = ui->UseSystemAccentColors->isChecked();
        }

        // If using system accent, prefer the stored ThemeAccentColor1 so the
        // preview matches the persisted setting; fall back to runtime system
        // accent if the parameter is not present.
        if (useSystem) {
            auto hGrpThemes = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Themes");
            unsigned long stored = hGrpThemes->GetUnsigned("ThemeAccentColor1", 0);
            if (stored != 0) {
                int a = (int)((stored >> 24) & 0xFF);
                int r = (int)((stored >> 16) & 0xFF);
                int g = (int)((stored >> 8) & 0xFF);
                int b = (int)(stored & 0xFF);
                if (a == 0) a = 255;
                accent = QColor(r, g, b, a);
            }
            if (!accent.isValid()) {
                accent = getSystemAccentColor();
            }
            // normalize alpha to opaque for consistent rendering
            accent.setAlpha(255);
            // ensure the pref button shows the same color even when disabled
            if (ui->ThemeAccentColor1) {
                ui->ThemeAccentColor1->setColor(accent);
            }
        } else {
            accent = getSystemAccentColor();
        }

        // set preview background
        if (ui->SystemAccentPreview) {
            // ensure opaque preview
            QColor previewColor = accent;
            previewColor.setAlpha(255);
            QString style = QStringLiteral("background-color: %1; border: 1px solid %2;")
                .arg(previewColor.name(QColor::HexArgb))
                .arg(previewColor.darker(250).name());
            ui->SystemAccentPreview->setStyleSheet(style);
        }

        // enable/disable pickers based on checkbox state — only disable
        // the primary accent picker. Secondary tones remain editable.
        if (ui->UseSystemAccentColors) {
            ui->ThemeAccentColor1->setEnabled(!useSystem);
            // keep ThemeAccentColor2/3 editable so users can tweak variants
            // even when following the OS accent for the primary tone.
            ui->ThemeAccentColor2->setEnabled(true);
            ui->ThemeAccentColor3->setEnabled(true);
            // gray out preview when not using system accent
            ui->SystemAccentPreview->setEnabled(useSystem);
            ui->SystemAccentPreview->setWindowOpacity(useSystem ? 1.0 : 0.45);
        }
    };

    if (ui->UseSystemAccentColors) {
        connect(ui->UseSystemAccentColors, &Gui::PrefCheckBox::toggled, this, [this, updateAccentUi](bool on){
            updateAccentUi();
            if (on) {
                // persist the system accent colors immediately into the Themes group
                QColor accent = getSystemAccentColor();
                auto hGrpThemes = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/Themes");
                // Use the same packed RGBA format as PrefColorButton
                unsigned int icol = Base::Color::asPackedRGBA<QColor>(accent);
                unsigned long val1 = static_cast<unsigned long>(icol);
                unsigned long val2 = val1; // start from same base, will darken/lighen using QColor
                unsigned long val3 = val1;
                QColor c2 = accent.darker(115);
                QColor c3 = accent.lighter(140);
                val2 = ((unsigned long)c2.alpha() << 24) | ((unsigned long)c2.red() << 16) |
                       ((unsigned long)c2.green() << 8) | (unsigned long)c2.blue();
                val3 = ((unsigned long)c3.alpha() << 24) | ((unsigned long)c3.red() << 16) |
                       ((unsigned long)c3.green() << 8) | (unsigned long)c3.blue();
                // Persist only the primary accent color. Windows exposes
                // multiple accent tones, but we only store ThemeAccentColor1
                // here to keep behavior deterministic and avoid overwriting
                // user-customized secondary tones.
                hGrpThemes->SetUnsigned("ThemeAccentColor1", val1);
                // Immediately restore the pref button from the parameter so
                // the UI matches the persisted value (avoids transient
                // mismatch between preview and button rendering).
                if (ui->ThemeAccentColor1) {
                    ui->ThemeAccentColor1->onRestore();
                }
            }
        });
    }
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
    ui->UseSystemAccentColors->onSave();
    ui->StyleSheets->onSave();
    ui->OverlayStyleSheets->onSave();

    // If using system accent colors, persist the computed values into the Themes group
    if (ui->UseSystemAccentColors && ui->UseSystemAccentColors->isChecked()) {
        QColor accent = getSystemAccentColor();
        auto hGrpThemes = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Themes");
    unsigned int icol = Base::Color::asPackedRGBA<QColor>(accent);
    unsigned long val1 = static_cast<unsigned long>(icol);
        QColor c2 = accent.darker(115);
        QColor c3 = accent.lighter(140);
        unsigned long val2 = ((unsigned long)c2.alpha() << 24) | ((unsigned long)c2.red() << 16) |
                            ((unsigned long)c2.green() << 8) | (unsigned long)c2.blue();
        unsigned long val3 = ((unsigned long)c3.alpha() << 24) | ((unsigned long)c3.red() << 16) |
                            ((unsigned long)c3.green() << 8) | (unsigned long)c3.blue();
    // Persist only the primary accent color (ThemeAccentColor1).
    hGrpThemes->SetUnsigned("ThemeAccentColor1", val1);
    }

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

    // TaskWatcher
    ui->showTaskWatcherCheckBox->onSave();
}

void DlgSettingsUI::loadSettings()
{
    // Theme
    ui->ThemeAccentColor1->onRestore();
    ui->ThemeAccentColor2->onRestore();
    ui->ThemeAccentColor3->onRestore();
    ui->UseSystemAccentColors->onRestore();

    // If available, override the accent color widgets with the OS accent color
    // so the UI matches the system look-and-feel. We compute two simple
    // variants for accent2/3 (darker/lighter).
    QColor systemAccent = getSystemAccentColor();

    if (systemAccent.isValid()) {
        // Only override if the stored theme accent is missing or still the
        // default that comes with the preference pack. This avoids clobbering
        // explicit user choices.
        auto hGrpThemes = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Themes");
        const unsigned long nonExistentColor = -1434171135;
        const unsigned long defaultAccentColor = 1434171135;
        unsigned long stored = hGrpThemes->GetUnsigned("ThemeAccentColor1", nonExistentColor);
        if (stored == nonExistentColor || stored == defaultAccentColor) {
            ui->ThemeAccentColor1->setColor(systemAccent);
            ui->ThemeAccentColor2->setColor(systemAccent.darker(115));
            ui->ThemeAccentColor3->setColor(systemAccent.lighter(140));
        }
    }

    // Respect the use-system-accent checkbox: if enabled, disable manual pickers
    bool useSystem = false;
    if (ui->UseSystemAccentColors) {
        // PrefCheckBox stores a boolean
        useSystem = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Themes")->GetBool("UseSystemAccentColors", false);
        ui->ThemeAccentColor1->setEnabled(!useSystem);
        ui->ThemeAccentColor2->setEnabled(!useSystem);
        ui->ThemeAccentColor3->setEnabled(!useSystem);
    }

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

    // TaskWatcher
    ui->showTaskWatcherCheckBox->onRestore();

    // update preview color and preview/picker enable state (initial)
    QColor systemAccentColor = getSystemAccentColor();
    if (ui->SystemAccentPreview) {
        QString style = QStringLiteral("background-color: %1; border: 1px solid %2;")
            .arg(systemAccentColor.name(QColor::HexArgb))
            .arg(systemAccentColor.darker(250).name());
        ui->SystemAccentPreview->setStyleSheet(style);
    }

    if (ui->UseSystemAccentColors) {
        // useSystem was earlier read to enable/disable pickers; maintain preview state
        bool useSystemPreview = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Themes")->GetBool("UseSystemAccentColors", false);
        if (ui->SystemAccentPreview) {
            ui->SystemAccentPreview->setEnabled(useSystemPreview);
            ui->SystemAccentPreview->setWindowOpacity(useSystemPreview ? 1.0 : 0.45);
        }
    }

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

// system accent helper (Windows DWM, macOS preferences, Qt fallback)
namespace {
QColor getSystemAccentColor()
{
#ifdef FC_OS_WIN32
    DWORD color = 0;
    BOOL opaque = FALSE;
    if (SUCCEEDED(DwmGetColorizationColor(&color, &opaque))) {
        int a = (int)((color >> 24) & 0xFF);
        int r = (int)((color >> 16) & 0xFF);
        int g = (int)((color >> 8) & 0xFF);
        int b = (int)(color & 0xFF);
        if (a == 0) a = 255;
        return QColor(r, g, b, a);
    }
#endif
#ifdef FC_OS_MACOSX
    if (auto val = CFPreferencesCopyAppValue(CFSTR("AppleHighlightColor"), kCFPreferencesAnyApplication)) {
        if (CFGetTypeID(val) == CFArrayGetTypeID()) {
            CFIndex count = CFArrayGetCount((CFArrayRef)val);
            if (count >= 3) {
                double r = 0, g = 0, b = 0;
                CFNumberRef num = (CFNumberRef)CFArrayGetValueAtIndex((CFArrayRef)val, 0);
                CFNumberGetValue(num, kCFNumberDoubleType, &r);
                num = (CFNumberRef)CFArrayGetValueAtIndex((CFArrayRef)val, 1);
                CFNumberGetValue(num, kCFNumberDoubleType, &g);
                num = (CFNumberRef)CFArrayGetValueAtIndex((CFArrayRef)val, 2);
                CFNumberGetValue(num, kCFNumberDoubleType, &b);
                CFRelease(val);
                return QColor::fromRgbF((float)r, (float)g, (float)b);
            }
        }
        CFRelease(val);
    }
    if (auto idxVal = CFPreferencesCopyAppValue(CFSTR("AppleAccentColor"), kCFPreferencesAnyApplication)) {
        if (CFGetTypeID(idxVal) == CFNumberGetTypeID()) {
            int idx = 0;
            CFNumberGetValue((CFNumberRef)idxVal, kCFNumberIntType, &idx);
            CFRelease(idxVal);
            switch (idx) {
                case 0: return QColor(128,128,128);
                case 1: return QColor(255,59,48);
                case 2: return QColor(255,149,0);
                case 3: return QColor(255,204,0);
                case 4: return QColor(52,199,89);
                case 5: return QColor(0,122,255);
                case 6: return QColor(88,86,214);
                default: break;
            }
        }
        CFRelease(idxVal);
    }
#endif
    return qApp->palette().color(QPalette::Highlight);
}
} // helper namespace

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

