/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "DlgSettingsObjectColor.h"
#include "ui_DlgSettingsObjectColor.h"


using namespace PartGui;

/* TRANSLATOR PartGui::DlgSettingsObjectColor */

/**
 *  Constructs a DlgSettingsObjectColor which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsObjectColor::DlgSettingsObjectColor(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsObjectColor)
{
    ui->setupUi(this);
    ui->DefaultShapeColor->setDisabled(ui->checkRandomColor->isChecked());
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsObjectColor::~DlgSettingsObjectColor() = default;

void DlgSettingsObjectColor::saveSettings()
{
    // Part
    ui->DefaultShapeColor->onSave();
    ui->checkRandomColor->onSave();
    ui->DefaultShapeTransparency->onSave();
    ui->DefaultShapeLineColor->onSave();
    ui->DefaultShapeLineWidth->onSave();
    ui->DefaultShapeVertexColor->onSave();
    ui->DefaultShapeVertexSize->onSave();
    ui->BoundingBoxColor->onSave();
    ui->BoundingBoxFontSize->onSave();
    ui->twosideRendering->onSave();
    // Annotations
    ui->AnnotationTextColor->onSave();
}

void DlgSettingsObjectColor::loadSettings()
{
    // Part
    ui->DefaultShapeColor->onRestore();
    ui->checkRandomColor->onRestore();
    ui->DefaultShapeTransparency->onRestore();
    ui->DefaultShapeLineColor->onRestore();
    ui->DefaultShapeLineWidth->onRestore();
    ui->DefaultShapeVertexColor->onRestore();
    ui->DefaultShapeVertexSize->onRestore();
    ui->BoundingBoxColor->onRestore();
    ui->BoundingBoxFontSize->onRestore();
    ui->twosideRendering->onRestore();
    // Annotations
    ui->AnnotationTextColor->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsObjectColor::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsObjectColor.cpp"

