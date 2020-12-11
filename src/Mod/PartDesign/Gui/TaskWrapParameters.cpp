/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QMessageBox>
#include <QCheckBox>
#endif

#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/FeatureWrap.h>

#include "ui_TaskWrapParameters.h"
#include "TaskWrapParameters.h"
#include "Utils.h"

using namespace PartDesignGui;
using namespace Gui;

TaskWrapParameters::TaskWrapParameters(PartDesignGui::ViewProviderWrap *vp, QWidget *parent)
    : TaskFeatureParameters(vp, parent, tr("PartDesign wrap"))
{
    connRecompute = App::GetApplication().signalRecomputedObject.connect(
        [this](const App::Document &doc, const App::DocumentObject &obj) {
            if (this->busy || !this->vp || doc.testStatus(App::Document::Recomputing))
                return;
            auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(this->vp->getObject());
            if (!feat || feat->WrapFeature.getValue() != &obj)
                return;
            Base::StateLocker lock(this->busy);
            this->recomputeFeature();
        });

    setupUI();
}

TaskWrapParameters::~TaskWrapParameters()
{
}

void TaskWrapParameters::setupUI()
{
    proxy = new QWidget(this);
    ui = new Ui_TaskWrapParameters();
    ui->setupUi(proxy);

    this->addNewSolidCheckBox(proxy);
    PartDesignGui::addTaskCheckBox(proxy);

    this->groupLayout()->addWidget(proxy);

    connect(ui->comboBoxType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onTypeChanged(int)));
    connect(ui->checkBoxFrozen, SIGNAL(toggled(bool)),
            this, SLOT(onFrozenChanged()));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(
            this->vp->getObject());
    if (feat) {
        ui->comboBoxType->setToolTip(QApplication::translate(
                    "App::Property", feat->Type.getDocumentation()));
        ui->checkBoxFrozen->setToolTip(QApplication::translate(
                    "App::Property", feat->Frozen.getDocumentation()));
    }

    refresh();
}

void TaskWrapParameters::refresh()
{
    if (!this->vp)
        return;

    TaskFeatureParameters::refresh();

    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(this->vp->getObject());
    if (!feat)
        return;

    {
        QSignalBlocker guard(ui->checkBoxFrozen);
        ui->checkBoxFrozen->setChecked(feat->Frozen.getValue());
    }
    {
        QSignalBlocker guard(ui->comboBoxType);
        ui->comboBoxType->setCurrentIndex(feat->Type.getValue());
    }
}

void TaskWrapParameters::onTypeChanged(int index)
{
    if (!vp)
        return;
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(this->vp->getObject());
    if (!feat)
        return;
    feat->Type.setValue(index);
    recomputeFeature();
}

void TaskWrapParameters::onFrozenChanged()
{
    if (!vp)
        return;
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(this->vp->getObject());
    if (!feat)
        return;
    feat->Frozen.setValue(ui->checkBoxFrozen->isChecked());
    recomputeFeature();
}

#include "moc_TaskWrapParameters.cpp"
