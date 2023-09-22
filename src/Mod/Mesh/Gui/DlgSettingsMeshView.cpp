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

#include <Gui/Window.h>

#include "DlgSettingsMeshView.h"
#include "ui_DlgSettingsMeshView.h"


using namespace MeshGui;

/**
 *  Constructs a DlgSettingsMeshView which is a child of 'parent'.
 */
DlgSettingsMeshView::DlgSettingsMeshView(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsMeshView)
{
    ui->setupUi(this);
    ui->labelBackfaceColor->hide();
    ui->buttonBackfaceColor->hide();
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsMeshView::~DlgSettingsMeshView() = default;

void DlgSettingsMeshView::saveSettings()
{
    ui->checkboxRendering->onSave();
    ui->checkboxBoundbox->onSave();
    ui->buttonMeshColor->onSave();
    ui->buttonLineColor->onSave();
    ui->buttonBackfaceColor->onSave();
    ui->spinMeshTransparency->onSave();
    ui->spinLineTransparency->onSave();
    ui->checkboxNormal->onSave();
    ui->spinboxAngle->onSave();
}

void DlgSettingsMeshView::loadSettings()
{
    Base::Reference<ParameterGrp> hGrp = Gui::WindowParameter::getDefaultParameter();
    hGrp = hGrp->GetGroup("View");
    if (!hGrp->GetBool("EnablePreselection", true) && !hGrp->GetBool("EnableSelection", true)) {
        ui->checkboxBoundbox->setDisabled(true);
    }
    ui->checkboxRendering->onRestore();
    ui->checkboxBoundbox->onRestore();
    ui->buttonMeshColor->onRestore();
    ui->buttonLineColor->onRestore();
    ui->buttonBackfaceColor->onRestore();
    ui->spinMeshTransparency->onRestore();
    ui->spinLineTransparency->onRestore();
    ui->checkboxNormal->onRestore();
    ui->spinboxAngle->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsMeshView::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsMeshView.cpp"
