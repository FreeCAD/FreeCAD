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

#include "DlgSettingsRayImp.h"
#include <Gui/PrefWidgets.h>
#include <Base/Console.h>

using namespace RaytracingGui;

/**
 *  Constructs a DlgSettings3DViewImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettingsRayImp::DlgSettingsRayImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsRayImp::~DlgSettingsRayImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsRayImp::saveSettings()
{
    prefFileChooser1->onSave();
    prefFileChooser2->onSave();
    prefFileChooser3->onSave();
    prefLineEdit2->onSave();
    prefLineEdit3->onSave();
    prefFloatSpinBox1->onSave();
    prefCheckBox8->onSave();
    prefCheckBox9->onSave();
    prefIntSpinBox1->onSave();
    prefIntSpinBox2->onSave();
    prefLineEdit1->onSave();
}

void DlgSettingsRayImp::loadSettings()
{
    prefFileChooser1->onRestore();
    prefFileChooser2->onRestore();
    prefFileChooser3->onRestore();
    prefLineEdit2->onRestore();
    prefLineEdit3->onRestore();
    prefFloatSpinBox1->onRestore();
    prefCheckBox8->onRestore();
    prefCheckBox9->onRestore();
    prefIntSpinBox1->onRestore();
    prefIntSpinBox2->onRestore();
    prefLineEdit1->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsRayImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsRayImp.cpp"
