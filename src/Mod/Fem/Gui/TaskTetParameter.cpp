/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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
# include <QString>
# include <QSlider>

# include <Standard_math.hxx>
#endif

#include "ui_TaskTetParameter.h"
#include "TaskTetParameter.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Utilities.h>
#include <Mod/Fem/App/FemMeshShapeNetgenObject.h>
#include <Mod/Fem/App/FemMesh.h>


using namespace FemGui;
using namespace Gui;


TaskTetParameter::TaskTetParameter(Fem::FemMeshShapeNetgenObject *pcObject,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("fem-femmesh-create-node-by-poly"),
      tr("Tet Parameter"),
      true,
      parent),
      pcObject(pcObject)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskTetParameter();
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

    QObject::connect(ui->doubleSpinBox_MaxSize,SIGNAL(valueChanged(double)),this,SLOT(maxSizeValueChanged(double)));
    QObject::connect(ui->comboBox_Fineness,SIGNAL(activated  (int)),this,SLOT(SwitchMethod(int)));
    QObject::connect(ui->checkBox_SecondOrder,SIGNAL(stateChanged  (int)),this,SLOT(setQuadric(int)));
    QObject::connect(ui->doubleSpinBox_GrowthRate,SIGNAL(valueChanged(double)),this,SLOT(setGrowthRate(double)));
    QObject::connect(ui->spinBox_SegsPerEdge,SIGNAL(valueChanged (int)),this,SLOT(setSegsPerEdge(int)));
    QObject::connect(ui->spinBox_SegsPerRadius,SIGNAL(valueChanged (int)),this,SLOT(setSegsPerRadius(int)));
    QObject::connect(ui->checkBox_Optimize,SIGNAL(stateChanged  (int)),this,SLOT(setOptimize(int)));

    if(pcObject->FemMesh.getValue().getInfo().numNode == 0)
        touched = true;
    else
        touched = false;

    setInfo();
}

TaskTetParameter::~TaskTetParameter()
{
    delete ui;
}

void TaskTetParameter::SwitchMethod(int Value)
{
    if(Value == 5){
        ui->doubleSpinBox_GrowthRate->setEnabled(true);
        ui->spinBox_SegsPerEdge->setEnabled(true);
        ui->spinBox_SegsPerRadius->setEnabled(true);
    }else{
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
    pcObject->SecondOrder.setValue(s!=0);
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
    pcObject->Optimize.setValue(v!=0);
    touched = true;
}


void TaskTetParameter::setInfo(void)
{
    Fem::FemMesh::FemMeshInfo info = pcObject->FemMesh.getValue().getInfo();
    //Base::BoundBox3d bndBox = pcObject->FemMesh.getValue().getBoundBox();


    ui->lineEdit_InfoNodes      ->setText(QString::number(info.numNode));
    ui->lineEdit_InfoTriangle   ->setText(QString::number(info.numFaces));
    ui->lineEdit_InfoTet        ->setText(QString::number(info.numTetr));
}





#include "moc_TaskTetParameter.cpp"
