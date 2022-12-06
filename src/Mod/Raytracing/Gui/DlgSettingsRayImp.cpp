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
#ifndef _PreComp_
# include <QApplication>
#endif

#include "DlgSettingsRayImp.h"
#include "ui_DlgSettingsRay.h"


using namespace RaytracingGui;

/* TRANSLATOR RaytracingGui::DlgSettingsRayImp */

/**
 *  Constructs a DlgSettingsRayImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettingsRayImp::DlgSettingsRayImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgSettingsRay)
{
    ui->setupUi(this);
    ui->prefFileChooser2->setToolTip(tr("The path to the POV-Ray executable, if you want to render from %1").arg(qApp->applicationName()));
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
    ui->prefFileChooser1->onSave();
    ui->prefFileChooser2->onSave();
    ui->prefFileChooser3->onSave();
    ui->prefLineEdit2->onSave();
    ui->prefLineEdit3->onSave();
    ui->prefFloatSpinBox1->onSave();
    ui->prefCheckBox8->onSave();
    ui->prefCheckBox9->onSave();
    ui->prefIntSpinBox1->onSave();
    ui->prefIntSpinBox2->onSave();
    ui->prefLineEdit1->onSave();
}

void DlgSettingsRayImp::loadSettings()
{
    ui->prefFileChooser1->onRestore();
    ui->prefFileChooser2->onRestore();
    ui->prefFileChooser3->onRestore();
    ui->prefLineEdit2->onRestore();
    ui->prefLineEdit3->onRestore();
    ui->prefFloatSpinBox1->onRestore();
    ui->prefCheckBox8->onRestore();
    ui->prefCheckBox9->onRestore();
    ui->prefIntSpinBox1->onRestore();
    ui->prefIntSpinBox2->onRestore();
    ui->prefLineEdit1->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsRayImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsRayImp.cpp"
