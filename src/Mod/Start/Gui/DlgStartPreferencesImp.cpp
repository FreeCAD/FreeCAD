/***************************************************************************
 *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
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

#include <Gui/Application.h>

#include "DlgStartPreferencesImp.h"
#include "ui_DlgStartPreferences.h"


using namespace StartGui;

/**
 *  Constructs a DlgStartPreferencesImp which is a child of 'parent'
 */
DlgStartPreferencesImp::DlgStartPreferencesImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgStartPreferences)
{
    ui->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgStartPreferencesImp::~DlgStartPreferencesImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgStartPreferencesImp::saveSettings()
{
    ui->fileChooserCustomFolder->onSave();
    ui->checkBoxShowExamples->onSave();
    ui->checkBoxCloseAfterLoading->onSave();
    ui->checkBoxShowOnlyFCStd->onSave();
}

void DlgStartPreferencesImp::loadSettings()
{
    ui->fileChooserCustomFolder->onRestore();
    ui->checkBoxShowExamples->onRestore();
    ui->checkBoxCloseAfterLoading->onRestore();
    ui->checkBoxShowOnlyFCStd->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgStartPreferencesImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgStartPreferencesImp.cpp"
