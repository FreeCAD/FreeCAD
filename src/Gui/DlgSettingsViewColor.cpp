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
#endif

#include "DlgSettingsViewColor.h"
#include "PrefWidgets.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsViewColor */

/**
 *  Constructs a DlgSettingsViewColor which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettingsViewColor::DlgSettingsViewColor(QWidget* parent)
    : PreferencePage(parent)
{
    this->setupUi(this);
    this->HighlightColor->setEnabled(this->checkBoxPreselection->isChecked());
    this->SelectionColor->setEnabled(this->checkBoxSelection->isChecked());
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsViewColor::~DlgSettingsViewColor()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsViewColor::saveSettings()
{
    SelectionColor_Background->onSave();
    backgroundColorFrom->onSave();
    backgroundColorTo->onSave();
    backgroundColorMid->onSave();
    radioButtonSimple->onSave();
    radioButtonGradient->onSave();
    checkMidColor->onSave();
    checkBoxPreselection->onSave();
    checkBoxSelection->onSave();
    HighlightColor->onSave();
    SelectionColor->onSave();
}

void DlgSettingsViewColor::loadSettings()
{
    SelectionColor_Background->onRestore();
    backgroundColorFrom->onRestore();
    backgroundColorTo->onRestore();
    backgroundColorMid->onRestore();
    radioButtonSimple->onRestore();
    radioButtonGradient->onRestore();
    checkMidColor->onRestore();
    checkBoxPreselection->onRestore();
    checkBoxSelection->onRestore();
    HighlightColor->onRestore();
    SelectionColor->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsViewColor::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsViewColor.cpp"

