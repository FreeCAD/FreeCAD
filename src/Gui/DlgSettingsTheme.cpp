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

#include "DlgSettingsTheme.h"
#include "ui_DlgSettingsTheme.h"
#include "Application.h"
#include "PreferencePackManager.h"


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
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsTheme::~DlgSettingsTheme()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsTheme::saveSettings()
{
    ui->ThemeSecondaryColor->onSave();
    ui->ThemeHighlightColor->onSave();
    ui->ThemeFocusColor->onSave();

    if (styleSheetChanged)
        saveStyleSheet();
}

void DlgSettingsTheme::loadSettings()
{
    ui->ThemeSecondaryColor->onRestore();
    ui->ThemeHighlightColor->onRestore();
    ui->ThemeFocusColor->onRestore();

    loadStyleSheet();
}

void DlgSettingsTheme::saveStyleSheet()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");

    QVariant sheet = ui->styleSheetsCombobox->itemData(ui->styleSheetsCombobox->currentIndex());
    hGrp->SetASCII("StyleSheet", (const char*)sheet.toByteArray());
    bool tiledBackground = hGrp->GetBool("TiledBackground", false);
    Application::Instance->setStyleSheet(sheet.toString(), tiledBackground);
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
    Base::Console().Warning("Hello");
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

