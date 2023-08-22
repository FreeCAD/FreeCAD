/***************************************************************************
 *   Copyright (c) 2022                                                    *
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

#include <Gui/Command.h>

#include "DlgSettingsMeasure.h"
#include "ui_DlgSettingsMeasure.h"


using namespace PartGui;

DlgSettingsMeasure::DlgSettingsMeasure(QWidget* parent)
  : PreferencePage(parent) , ui(new Ui_DlgSettingsMeasure)
{
    ui->setupUi(this);
    connect(ui->pushButtonRefresh, &QPushButton::clicked, this, &DlgSettingsMeasure::onMeasureRefresh);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsMeasure::~DlgSettingsMeasure() = default;

void DlgSettingsMeasure::saveSettings()
{
    ui->dim3dColorButton->onSave();
    ui->dimDeltaColorButton->onSave();
    ui->dimAngularColorButton->onSave();

    ui->fontSizeSpinBox->onSave();
    ui->fontNameComboBox->onSave();

    ui->fontStyleBoldCheckBox->onSave();
    ui->fontStyleItalicCheckBox->onSave();
}

void DlgSettingsMeasure::loadSettings()
{
    ui->dim3dColorButton->onRestore();
    ui->dimDeltaColorButton->onRestore();
    ui->dimAngularColorButton->onRestore();

    ui->fontSizeSpinBox->onRestore();
    ui->fontNameComboBox->onRestore();
    ui->fontNameComboBox->addItems(QStringList({QString::fromUtf8("defaultFont")}));

    ui->fontStyleBoldCheckBox->onRestore();
    ui->fontStyleItalicCheckBox->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsMeasure::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsMeasure::onMeasureRefresh()
{
    DlgSettingsMeasure::saveSettings();
    Gui::Command::runCommand(Gui::Command::Gui, "Gui.runCommand('Part_Measure_Refresh',0)");
}

#include "moc_DlgSettingsMeasure.cpp"
