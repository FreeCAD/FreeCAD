/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: WandererFan <wandererfan@gmail.com>                           *
 *   Based on src/Mod/FEM/Gui/DlgSettingsFEMImp.cpp                        *
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

#include "DlgPrefsTechDrawScaleImp.h"
#include "ui_DlgPrefsTechDrawScale.h"


using namespace TechDrawGui;

DlgPrefsTechDrawScaleImp::DlgPrefsTechDrawScaleImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgPrefsTechDrawScaleImp)
{
    ui->setupUi(this);

    ui->pdsbTemplateMark->setUnit(Base::Unit::Length);
    ui->pdsbTemplateMark->setMinimum(0);

    connect(ui->cbViewScaleType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgPrefsTechDrawScaleImp::onScaleTypeChanged);
}

DlgPrefsTechDrawScaleImp::~DlgPrefsTechDrawScaleImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawScaleImp::onScaleTypeChanged(int index)
{
    // disable custom scale if the scale type is not custom

    if (index == 2) // if custom
        ui->pdsbViewScale->setEnabled(true);
    else
        ui->pdsbViewScale->setEnabled(false);
}

void DlgPrefsTechDrawScaleImp::saveSettings()
{
    ui->pdsbPageScale->onSave();
    ui->cbViewScaleType->onSave();
    ui->pdsbViewScale->onSave();
    ui->pdsbVertexScale->onSave();
    ui->pdsbCenterScale->onSave();
    ui->pdsbTemplateMark->onSave();
    ui->pdsbSymbolScale->onSave();
}

void DlgPrefsTechDrawScaleImp::loadSettings()
{
    ui->pdsbPageScale->onRestore();
    ui->cbViewScaleType->onRestore();
    ui->pdsbViewScale->onRestore();
    ui->pdsbVertexScale->onRestore();
    ui->pdsbCenterScale->onRestore();
    double markDefault = 3.0;
    ui->pdsbTemplateMark->setValue(markDefault);
    ui->pdsbTemplateMark->onRestore();
    ui->pdsbSymbolScale->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawScaleImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        saveSettings();
        ui->retranslateUi(this);
        loadSettings();
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawScaleImp.cpp>
