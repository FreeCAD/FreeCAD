/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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

#include "DlgSettingsPathColor.h"
#include <Gui/PrefWidgets.h>
#include <Base/Console.h>

using namespace PathGui;

/* TRANSLATOR PathGui::DlgSettingsPathColor */

/**
 *  Constructs a DlgSettingsObjectColor which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsPathColor::DlgSettingsPathColor(QWidget* parent)
    : PreferencePage(parent)
{
    this->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsPathColor::~DlgSettingsPathColor()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsPathColor::saveSettings()
{
    // Part
    DefaultNormalPathColor->onSave();
    DefaultRapidPathColor->onSave();
    DefaultPathLineWidth->onSave();
    DefaultPathMarkerColor->onSave();
    DefaultExtentsColor->onSave();
    DefaultProbePathColor->onSave();
    DefaultHighlightPathColor->onSave();
    DefaultBBoxSelectionColor->onSave();
    DefaultBBoxNormalColor->onSave();
	DefaultSelectionStyle->onSave();
}

void DlgSettingsPathColor::loadSettings()
{
    // Part
    DefaultNormalPathColor->onRestore();
    DefaultRapidPathColor->onRestore();
    DefaultPathLineWidth->onRestore();
    DefaultPathMarkerColor->onRestore();
    DefaultExtentsColor->onRestore();
    DefaultProbePathColor->onRestore();
    DefaultHighlightPathColor->onRestore();
    DefaultBBoxSelectionColor->onRestore();
    DefaultBBoxNormalColor->onRestore();
	DefaultSelectionStyle->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsPathColor::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsPathColor.cpp"

