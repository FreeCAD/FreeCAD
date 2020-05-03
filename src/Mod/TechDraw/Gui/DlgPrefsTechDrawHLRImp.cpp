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

#include "DlgPrefsTechDrawHLRImp.h"
#include <Gui/PrefWidgets.h>

using namespace TechDrawGui;

DlgPrefsTechDrawHLRImp::DlgPrefsTechDrawHLRImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
}

DlgPrefsTechDrawHLRImp::~DlgPrefsTechDrawHLRImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawHLRImp::saveSettings()
{
    pcbSeamViz->onSave();
    pcbSmoothViz->onSave();
    pcbHardViz->onSave();
    pcbPolygon->onSave();
    pcbIsoViz->onSave();
    pcbSmoothHid->onSave();
    pcbSeamHid->onSave();
    pcbIsoHid->onSave();
    psbIsoCount->onSave();
    pcbHardHid->onSave();
}

void DlgPrefsTechDrawHLRImp::loadSettings()
{
    pcbSeamViz->onRestore();
    pcbSmoothViz->onRestore();
    pcbHardViz->onRestore();
    pcbPolygon->onRestore();
    pcbIsoViz->onRestore();
    pcbSmoothHid->onRestore();
    pcbSeamHid->onRestore();
    pcbIsoHid->onRestore();
    psbIsoCount->onRestore();
    pcbHardHid->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawHLRImp::changeEvent(QEvent *e)
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

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawHLRImp.cpp>
