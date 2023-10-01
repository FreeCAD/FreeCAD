/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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
#include <QString>
#endif

#include <Gui/BitmapFactory.h>
#include <Mod/Fem/App/FemMesh.h>
#include <Mod/Fem/App/FemMeshShapeNetgenObject.h>

#include "TaskTetParameter.h"
#include "ui_TaskTetParameter.h"


using namespace FemGui;
using namespace Gui;

TaskTetParameter::TaskTetParameter(Fem::FemMeshShapeNetgenObject* pcObject, QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap("FEM_CreateNodesSet"), tr("Tet Parameter"), true, parent)
    , pcObject(pcObject)
    , ui(new Ui_TaskTetParameter)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->doubleSpinBox_MaxSize->setValue(pcObject->MaxSize.getValue());
    ui->comboBox_Fineness->setCurrentIndex(pcObject->Fineness.getValue());
    ui->checkBox_SecondOrder->setChecked(pcObject->SecondOrder.getValue());
    ui->doubleSpinBox_GrowthRate->setValue(pcObject->GrowthRate.getValue());
    ui->spinBox_SegsPerEdge->setValue(pcObject->NbSegsPerEdge.getValue());
    ui->spinBox_SegsPerRadius->setValue(pcObject->NbSegsPerRadius.getValue());
    ui->checkBox_Optimize->setChecked(pcObject->Optimize.getValue());

    QObject::connect(ui->doubleSpinBox_MaxSize,
                     qOverload<double>(&QDoubleSpinBox::valueChanged),
                     this,
                     &TaskTetParameter::maxSizeValueChanged);
    QObject::connect(ui->comboBox_Fineness,
                     qOverload<int>(&QComboBox::activated),
                     this,
                     &TaskTetParameter::SwitchMethod);
    QObject::connect(ui->checkBox_SecondOrder,
                     &QCheckBox::stateChanged,
                     this,
                     &TaskTetParameter::setQuadric);
    QObject::connect(ui->doubleSpinBox_GrowthRate,
                     qOverload<double>(&QDoubleSpinBox::valueChanged),
                     this,
                     &TaskTetParameter::setGrowthRate);
    QObject::connect(ui->spinBox_SegsPerEdge,
                     qOverload<int>(&QSpinBox::valueChanged),
                     this,
                     &TaskTetParameter::setSegsPerEdge);
    QObject::connect(ui->spinBox_SegsPerRadius,
                     qOverload<int>(&QSpinBox::valueChanged),
                     this,
                     &TaskTetParameter::setSegsPerRadius);
    QObject::connect(ui->checkBox_Optimize,
                     &QCheckBox::stateChanged,
                     this,
                     &TaskTetParameter::setOptimize);

    if (pcObject->FemMesh.getValue().getInfo().numNode == 0) {
        touched = true;
    }
    else {
        touched = false;
    }

    setInfo();
}

TaskTetParameter::~TaskTetParameter() = default;

void TaskTetParameter::SwitchMethod(int Value)
{
    if (Value == 5) {
        ui->doubleSpinBox_GrowthRate->setEnabled(true);
        ui->spinBox_SegsPerEdge->setEnabled(true);
        ui->spinBox_SegsPerRadius->setEnabled(true);
    }
    else {
        ui->doubleSpinBox_GrowthRate->setEnabled(false);
        ui->spinBox_SegsPerEdge->setEnabled(false);
        ui->spinBox_SegsPerRadius->setEnabled(false);
    }

    pcObject->Fineness.setValue(Value);
    touched = true;
}

void TaskTetParameter::maxSizeValueChanged(double Value)
{
    pcObject->MaxSize.setValue(Value);
    touched = true;
}

void TaskTetParameter::setQuadric(int s)
{
    pcObject->SecondOrder.setValue(s != 0);
    touched = true;
}

void TaskTetParameter::setGrowthRate(double v)
{
    pcObject->GrowthRate.setValue(v);
    touched = true;
}

void TaskTetParameter::setSegsPerEdge(int v)
{
    pcObject->NbSegsPerEdge.setValue(v);
    touched = true;
}

void TaskTetParameter::setSegsPerRadius(int v)
{
    pcObject->NbSegsPerRadius.setValue(v);
    touched = true;
}

void TaskTetParameter::setOptimize(int v)
{
    pcObject->Optimize.setValue(v != 0);
    touched = true;
}


void TaskTetParameter::setInfo()
{
    Fem::FemMesh::FemMeshInfo info = pcObject->FemMesh.getValue().getInfo();
    // Base::BoundBox3d bndBox = pcObject->FemMesh.getValue().getBoundBox();
    ui->lineEdit_InfoNodes->setText(QString::number(info.numNode));
    ui->lineEdit_InfoTriangle->setText(QString::number(info.numFaces));
    ui->lineEdit_InfoTet->setText(QString::number(info.numTetr));
}


#include "moc_TaskTetParameter.cpp"
