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

#ifndef _PreComp_
#endif

#include "DlgSettingsObjectColor.h"
#include <Gui/PrefWidgets.h>

using namespace PartGui;

/* TRANSLATOR PartGui::DlgSettingsObjectColor */

/**
 *  Constructs a DlgSettingsObjectColor which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettingsObjectColor::DlgSettingsObjectColor(QWidget* parent)
    : PreferencePage(parent)
{
    this->setupUi(this);
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsObjectColor::~DlgSettingsObjectColor()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsObjectColor::saveSettings()
{
    CursorTextColor->onSave();
    EditedEdgeColor->onSave();
    EditedVertexColor->onSave();
    ConstructionColor->onSave();
    FullyConstrainedColor->onSave();
    BoundingBoxColor->onSave();
    DefaultShapeColor->onSave();
    DefaultShapeLineColor->onSave();
    DefaultShapeLineWidth->onSave();
}

void DlgSettingsObjectColor::loadSettings()
{
    CursorTextColor->onRestore();
    EditedEdgeColor->onRestore();
    EditedVertexColor->onRestore();
    ConstructionColor->onRestore();
    FullyConstrainedColor->onRestore();
    BoundingBoxColor->onRestore();
    DefaultShapeColor->onRestore();
    DefaultShapeLineColor->onRestore();
    DefaultShapeLineWidth->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsObjectColor::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsObjectColor.cpp"

