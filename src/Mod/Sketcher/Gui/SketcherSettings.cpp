/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "SketcherSettings.h"
#include "ui_SketcherSettings.h"
#include "TaskSketcherGeneral.h"
#include <Gui/PrefWidgets.h>

using namespace SketcherGui;

/* TRANSLATOR SketcherGui::SketcherSettings */

SketcherSettings::SketcherSettings(QWidget* parent)
    : PreferencePage(parent), ui(new Ui_SketcherSettings)
{
    ui->setupUi(this);
    form = new SketcherGeneralWidget(ui->groupBox);
    ui->gridLayout->addWidget(form, 1, 0, 1, 1);

    // Don't need them at the moment
    ui->label_16->hide();
    ui->SketcherDatumWidth->hide();
    ui->label_12->hide();
    ui->DefaultSketcherVertexWidth->hide();
    ui->label_13->hide();
    ui->DefaultSketcherLineWidth->hide();
}

/** 
 *  Destroys the object and frees any allocated resources
 */
SketcherSettings::~SketcherSettings()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void SketcherSettings::saveSettings()
{
    // Sketcher
    ui->SketchEdgeColor->onSave();
    ui->SketchVertexColor->onSave();
    ui->EditedEdgeColor->onSave();
    ui->EditedVertexColor->onSave();
    ui->ConstructionColor->onSave();
    ui->ExternalColor->onSave();
    ui->FullyConstrainedColor->onSave();

    ui->ConstrainedColor->onSave();
    ui->DatumColor->onSave();

    ui->SketcherDatumWidth->onSave();
    ui->DefaultSketcherVertexWidth->onSave();
    ui->DefaultSketcherLineWidth->onSave();

    ui->CursorTextColor->onSave();

    // Sketch editing
    ui->EditSketcherFontSize->onSave();
    form->saveSettings();
}

void SketcherSettings::loadSettings()
{
    // Sketcher
    ui->SketchEdgeColor->onRestore();
    ui->SketchVertexColor->onRestore();
    ui->EditedEdgeColor->onRestore();
    ui->EditedVertexColor->onRestore();
    ui->ConstructionColor->onRestore();
    ui->ExternalColor->onRestore();
    ui->FullyConstrainedColor->onRestore();

    ui->ConstrainedColor->onRestore();
    ui->DatumColor->onRestore();

    ui->SketcherDatumWidth->onRestore();
    ui->DefaultSketcherVertexWidth->onRestore();
    ui->DefaultSketcherLineWidth->onRestore();

    ui->CursorTextColor->onRestore();

    // Sketch editing
    ui->EditSketcherFontSize->onRestore();
    form->loadSettings();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void SketcherSettings::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_SketcherSettings.cpp"

