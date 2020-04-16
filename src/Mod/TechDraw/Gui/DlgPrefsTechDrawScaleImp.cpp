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
#include <Gui/PrefWidgets.h>

using namespace TechDrawGui;

DlgPrefsTechDrawScaleImp::DlgPrefsTechDrawScaleImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);

    this->pdsbTemplateMark->setUnit(Base::Unit::Length);
    this->pdsbTemplateMark->setMinimum(0);

    connect(this->cbViewScaleType, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onScaleTypeChanged(int)));
}

DlgPrefsTechDrawScaleImp::~DlgPrefsTechDrawScaleImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawScaleImp::onScaleTypeChanged(int index)
{
    // disable custom scale if the scale type is not custom

    if (index == 2) // if custom
        this->pdsbViewScale->setEnabled(true);
    else
        this->pdsbViewScale->setEnabled(false);
}

void DlgPrefsTechDrawScaleImp::saveSettings()
{
    pdsbToleranceScale->onSave();
    pdsbTemplateMark->onSave();
    pdsbVertexScale->onSave();
    pdsbCenterScale->onSave();
    pdsbPageScale->onSave();
    cbViewScaleType->onSave();
    pdsbViewScale->onSave();
    pdsbEdgeFuzz->onSave();
    pdsbMarkFuzz->onSave();
    pdsbTemplateMark->onSave();
    pdsbSymbolScale->onSave();
}

void DlgPrefsTechDrawScaleImp::loadSettings()
{
    double markDefault = 3.0;
    pdsbTemplateMark->setValue(markDefault);
    pdsbToleranceScale->onRestore();
    pdsbTemplateMark->onRestore();
    pdsbVertexScale->onRestore();
    pdsbCenterScale->onRestore();
    pdsbPageScale->onRestore();
    cbViewScaleType->onRestore();
    pdsbViewScale->onRestore();
    pdsbEdgeFuzz->onRestore();
    pdsbMarkFuzz->onRestore();
    pdsbTemplateMark->onRestore();
    pdsbSymbolScale->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawScaleImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        saveSettings();
        retranslateUi(this);
        loadSettings();
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawScaleImp.cpp>
