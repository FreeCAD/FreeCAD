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

#include "ui_TaskAnalysisInfo.h"
#include "TaskAnalysisInfo.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>



using namespace FemGui;
using namespace Gui;


TaskAnalysisInfo::TaskAnalysisInfo(Fem::FemAnalysis *pcObject,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("fem-analysis"),
      tr("Nodes set"),
      true,
      parent),
      pcObject(pcObject)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskAnalysisInfo();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

 /*   QObject::connect(ui->toolButton_Poly,SIGNAL(clicked()),this,SLOT(Poly()));
    QObject::connect(ui->toolButton_Pick,SIGNAL(clicked()),this,SLOT(Pick()));
    QObject::connect(ui->comboBox,SIGNAL(activated  (int)),this,SLOT(SwitchMethod(int)));*/

}


void TaskAnalysisInfo::SwitchMethod(int /*Value*/)
{
 /*   if(Value == 1){
        ui->groupBox_AngleSearch->setEnabled(true);
        ui->toolButton_Pick->setEnabled(true);
        ui->toolButton_Poly->setEnabled(false);
    }else{
        ui->groupBox_AngleSearch->setEnabled(false);
        ui->toolButton_Pick->setEnabled(false);
        ui->toolButton_Poly->setEnabled(true);
    }*/
}


TaskAnalysisInfo::~TaskAnalysisInfo()
{
    delete ui;
}


#include "moc_TaskAnalysisInfo.cpp"
