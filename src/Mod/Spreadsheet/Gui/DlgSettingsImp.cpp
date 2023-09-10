/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Gui/Application.h>

#include "DlgSettingsImp.h"
#include "ui_DlgSettings.h"


using namespace SpreadsheetGui;

/* TRANSLATOR SpreadsheetGui::DlgSettingsImp */

DlgSettingsImp::DlgSettingsImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettings)
{
    ui->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsImp::~DlgSettingsImp() = default;

void DlgSettingsImp::saveSettings()
{

    /** use whatever the user has entered here
     *  we'll check for validity during import/export
     */
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Spreadsheet");
    QString delimiter = ui->delimiterComboBox->currentText();
    hGrp->SetASCII("ImportExportDelimiter", delimiter.toStdString().c_str());
    ui->quoteCharLineEdit->onSave();
    ui->escapeCharLineEdit->onSave();
    ui->formatString->onSave();
    ui->checkBoxShowAlias->onSave();
}

void DlgSettingsImp::loadSettings()
{
    /** items "tab", ";", and "," have already been added to the combo box in the .ui file
     *  we'll recognize a few tokens: comma, semicolon, tab, and \t
     */

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Spreadsheet");
    QString delimiter = QString::fromStdString(hGrp->GetASCII("ImportExportDelimiter", "tab"));
    int idx = ui->delimiterComboBox->findText(delimiter, Qt::MatchFixedString);
    if (idx != -1) {
        ui->delimiterComboBox->setCurrentIndex(idx);
    }
    else if (delimiter.compare(QLatin1String("\\t"), Qt::CaseInsensitive) == 0) {
        idx = ui->delimiterComboBox->findText(QLatin1String("tab"), Qt::MatchFixedString);
        ui->delimiterComboBox->setCurrentIndex(idx);
    }
    else if (delimiter.compare(QLatin1String("semicolon"), Qt::CaseInsensitive) == 0) {
        idx = ui->delimiterComboBox->findText(QLatin1String(";"), Qt::MatchFixedString);
        ui->delimiterComboBox->setCurrentIndex(idx);
    }
    else if (delimiter.compare(QLatin1String("comma"), Qt::CaseInsensitive) == 0) {
        idx = ui->delimiterComboBox->findText(QLatin1String(","), Qt::MatchFixedString);
        ui->delimiterComboBox->setCurrentIndex(idx);
    }
    else {
        ui->delimiterComboBox->addItem(delimiter);
        idx = ui->delimiterComboBox->findText(delimiter, Qt::MatchFixedString);
        ui->delimiterComboBox->setCurrentIndex(idx);
    }

    ui->quoteCharLineEdit->onRestore();
    ui->escapeCharLineEdit->onRestore();
    ui->formatString->onRestore();
    ui->checkBoxShowAlias->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsImp.cpp"
