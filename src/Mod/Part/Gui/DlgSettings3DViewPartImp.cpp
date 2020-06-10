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

#include <Python.h>
#include <QMessageBox>
#include "ViewProvider.h"
#include "DlgSettings3DViewPartImp.h"
#include "ui_DlgSettings3DViewPart.h"

#include <Gui/PrefWidgets.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include "PartParams.h"

using namespace PartGui;

/**
 *  Constructs a DlgSettings3DViewPart which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettings3DViewPart::DlgSettings3DViewPart(QWidget* parent)
  : PreferencePage(parent), ui(new Ui_DlgSettings3DViewPart), checkValue(false)
{
    ui->setupUi(this);

    ui->maxDeviation->setMinimum(PartParams::MinimumDeviation());
    ui->maxDeviation->setValue(PartParams::MeshDeviation());
    ui->maxAngularDeflection->setMinimum(PartParams::MinimumAngularDeflection());
    ui->maxAngularDeflection->setValue(PartParams::MeshAngularDeflection());
    ui->deviationLowerBound->setValue(PartParams::MinimumDeviation());
    ui->angularDeflectionLowerBound->setValue(PartParams::MinimumAngularDeflection());
    ui->checkBoxMapFaceColor->setChecked(PartParams::MapFaceColor());
    ui->checkBoxMapLineColor->setChecked(PartParams::MapLineColor());
    ui->checkBoxMapPointColor->setChecked(PartParams::MapPointColor());
    ui->checkBoxMapTransparency->setChecked(PartParams::MapTransparency());

    connect(ui->deviationLowerBound, SIGNAL(valueChanged(double)), this, SLOT(onLowerBoundChanged()));
    connect(ui->angularDeflectionLowerBound, SIGNAL(valueChanged(double)), this, SLOT(onLowerBoundChanged()));
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettings3DViewPart::~DlgSettings3DViewPart()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettings3DViewPart::on_maxDeviation_valueChanged(double v)
{
    if (!this->isVisible())
        return;
    if (v < 0.01 && !checkValue) {
        checkValue = true;
        QMessageBox::warning(this, tr("Deviation"),
            tr("Setting a too small deviation causes the tessellation to take longer"
               "and thus freezes or slows down the GUI."));
    }
}

void DlgSettings3DViewPart::onLowerBoundChanged()
{
    ui->maxDeviation->setMinimum(ui->deviationLowerBound->value());
    ui->maxAngularDeflection->setMinimum(ui->angularDeflectionLowerBound->value());
}

void DlgSettings3DViewPart::saveSettings()
{
    ui->maxDeviation->onSave();
    ui->maxAngularDeflection->onSave();
    ui->deviationLowerBound->onSave();
    ui->angularDeflectionLowerBound->onSave();
    ui->checkBoxOverride->onSave();
    ui->checkBoxMapFaceColor->onSave();
    ui->checkBoxMapLineColor->onSave();
    ui->checkBoxMapPointColor->onSave();
    ui->checkBoxMapTransparency->onSave();
}

void DlgSettings3DViewPart::loadSettings()
{
    ui->maxDeviation->onRestore();
    ui->maxAngularDeflection->onRestore();
    ui->deviationLowerBound->onRestore();
    ui->angularDeflectionLowerBound->onRestore();
    ui->checkBoxOverride->onRestore();
    ui->checkBoxMapFaceColor->onRestore();
    ui->checkBoxMapLineColor->onRestore();
    ui->checkBoxMapPointColor->onRestore();
    ui->checkBoxMapTransparency->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettings3DViewPart::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettings3DViewPartImp.cpp"
