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
#endif

#include <Standard_math.hxx>
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


using namespace FemGui;
using namespace Gui;


TaskTetParameter::TaskTetParameter(Fem::FemMeshShapeNetgenObject *pcObject,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Fem_FemMesh_createnodebypoly"),
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

    QObject::connect(ui->doubleSpinBox_MaxSize,SIGNAL(valueChanged(double)),this,SLOT(maxSizeValueChanged(double)));
    QObject::connect(ui->comboBox_Fineness,SIGNAL(activated  (int)),this,SLOT(SwitchMethod(int)));
    QObject::connect(ui->checkBox_SecondOrder,SIGNAL(stateChanged  (int)),this,SLOT(setQuadric(int)));
    QObject::connect(ui->doubleSpinBox_GrothRate,SIGNAL(valueChanged(double)),this,SLOT(setGrothRate(double)));
    QObject::connect(ui->spinBox_SegsPerEdge,SIGNAL(valueChanged (int)),this,SLOT(setSegsPerEdge(int)));
    QObject::connect(ui->spinBox_SegsPerRadius,SIGNAL(valueChanged (int)),this,SLOT(setSegsPerRadius(int)));
    QObject::connect(ui->checkBox_Optimize,SIGNAL(stateChanged  (int)),this,SLOT(setOptimize(int)));


}

TaskTetParameter::~TaskTetParameter()
{
    delete ui;
}

void TaskTetParameter::SwitchMethod(int Value)
{
    if(Value == 5){
        ui->doubleSpinBox_GrothRate->setEnabled(true);
        ui->spinBox_SegsPerEdge->setEnabled(true);
        ui->spinBox_SegsPerRadius->setEnabled(true);
    }else{
        ui->doubleSpinBox_GrothRate->setEnabled(false);
        ui->spinBox_SegsPerEdge->setEnabled(false);
        ui->spinBox_SegsPerRadius->setEnabled(false);
    }

    pcObject->Fininess.setValue(Value);
}

void TaskTetParameter::maxSizeValueChanged(double Value)
{
    pcObject->MaxSize.setValue(Value);
}

void TaskTetParameter::setQuadric(int s)
{
    pcObject->SecondOrder.setValue(s!=0);
}

void TaskTetParameter::setGrothRate(double v)
{
    pcObject->GrothRate.setValue(v);
}

void TaskTetParameter::setSegsPerEdge(int v)
{
    pcObject->NbSegsPerEdge.setValue(v);

}

void TaskTetParameter::setSegsPerRadius(int v)
{
    pcObject->NbSegsPerRadius.setValue(v);

}

void TaskTetParameter::setOptimize(int v)
{
    pcObject->Optimize.setValue(v!=0);

}







#include "moc_TaskTetParameter.cpp"
