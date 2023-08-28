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

#include "DlgSettingsTheme.h"
#include "ui_DlgSettingsTheme.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsTheme */

/**
 *  Constructs a DlgSettingsTheme which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsTheme::DlgSettingsTheme(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsTheme)
    , styleSheetChanged(false)
{
    ui->setupUi(this);

    connect(ui->styleSheetsCombobox, qOverload<int>(&QComboBox::activated), this, &DlgSettingsTheme::onStyleSheetChanged);
    connect(ui->ThemeAccentColor1, &Gui::PrefColorButton::changed, this, &DlgSettingsTheme::onColorChanged);
    connect(ui->ThemeAccentColor2, &Gui::PrefColorButton::changed, this, &DlgSettingsTheme::onColorChanged);
    connect(ui->ThemeAccentColor3, &Gui::PrefColorButton::changed, this, &DlgSettingsTheme::onColorChanged);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsTheme::~DlgSettingsTheme() = default;

void DlgSettingsTheme::saveSettings()
{
    ui->ThemeAccentColor1->onSave();
    ui->ThemeAccentColor2->onSave();
    ui->ThemeAccentColor3->onSave();

    if (styleSheetChanged)
        saveStyleSheet();
}

void DlgSettingsTheme::loadSettings()
{
    ui->ThemeAccentColor1->onRestore();
    ui->ThemeAccentColor2->onRestore();
    ui->ThemeAccentColor3->onRestore();

    loadStyleSheet();
}

void DlgSettingsTheme::saveStyleSheet()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");

    QVariant sheet = ui->styleSheetsCombobox->itemData(ui->styleSheetsCombobox->currentIndex());
    hGrp->SetASCII("StyleSheet", (const char*)sheet.toByteArray());
    bool tiledBackground = hGrp->GetBool("TiledBackground", false);
    Application::Instance->setStyleSheet(sheet.toString(), tiledBackground);

    styleSheetChanged = false;
}

void DlgSettingsTheme::loadStyleSheet()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");

    // List all .qss/.css files
    QMap<QString, QString> cssFiles;
    QDir dir;
    QStringList filter;
    filter << QString::fromLatin1("*.qss");
    filter << QString::fromLatin1("*.css");

    // read from user, resource and built-in directory
    QStringList qssPaths = QDir::searchPaths(QString::fromLatin1("qss"));
    for (const auto& qssPath : qssPaths) {
        dir.setPath(qssPath);
        QFileInfoList fileNames = dir.entryInfoList(filter, QDir::Files, QDir::Name);
        for (const auto& fileName : qAsConst(fileNames)) {
            if (cssFiles.find(fileName.baseName()) == cssFiles.end()) {
                cssFiles[fileName.baseName()] = fileName.fileName();
            }
        }
    }

    // now add all unique items
    ui->styleSheetsCombobox->clear();
    ui->styleSheetsCombobox->addItem(tr("No style sheet"), QString::fromLatin1(""));
    for (QMap<QString, QString>::iterator it = cssFiles.begin(); it != cssFiles.end(); ++it) {
        ui->styleSheetsCombobox->addItem(it.key(), it.value());
    }

    QString selectedStyleSheet = QString::fromLatin1(hGrp->GetASCII("StyleSheet").c_str());
    int index = ui->styleSheetsCombobox->findData(selectedStyleSheet);

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
                ui->styleSheetsCombobox->addItem(fi.baseName(), selectedStyleSheet);
            }

            index = ui->styleSheetsCombobox->findData(selectedStyleSheet);
        }
    }

    if (index > -1)
        ui->styleSheetsCombobox->setCurrentIndex(index);
}

void DlgSettingsTheme::onStyleSheetChanged(int index) {
    Q_UNUSED(index);
    styleSheetChanged = true;
}

void DlgSettingsTheme::onColorChanged() {
    styleSheetChanged = true;
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsTheme::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}


#include "moc_DlgSettingsTheme.cpp"

