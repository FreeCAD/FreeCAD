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
#endif

#include <QString>
#include <QSlider>
#include "ui_TaskObjectName.h"
#include "TaskObjectName.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <App/DocumentObject.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>


using namespace FemGui;
using namespace Gui;

TaskObjectName::TaskObjectName(App::DocumentObject *pcObject,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Fem_FemMesh"),
      tr("TaskObjectName"),
      true, 
      parent),
      pcObject(pcObject)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskObjectName();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    //QObject::connect(ui->pushButton_HideShow,SIGNAL(clicked()),this,SLOT(hideShow()));
    //QObject::connect(ui->doubleSpinBoxSizing,SIGNAL(valueChanged (double)),this,SLOT(sizingValueChanged(double)));
    //QObject::connect(ui->checkBoxOrientation,SIGNAL(toggled  (bool)),this,SLOT(orientationToggled(bool)));

}
//void TaskObjectName::setHideShowObject(void)
//{
//    HideShowObj = pcObject->Source.getValue();
//
//    if(HideShowObj){
//        QString ObjectName = QString::fromUtf8(HideShowObj->Label.getValue());
//        ui->lineEdit_ObjectName->setText(ObjectName);
//    }else{
//        ui->lineEdit_ObjectName->setText(QString());
//    }
//}
//
//void TaskObjectName::hideShow(void)
//{
//    setHideShowObject();
//
//    if(HideShowObj){
//        Gui::Document* doc = Gui::Application::Instance->activeDocument();
//        if(doc->getViewProvider(HideShowObj)->isVisible())
//            doc->getViewProvider(HideShowObj)->setVisible(false);
//        else
//            doc->getViewProvider(HideShowObj)->setVisible(true);
//    }
//}
//
//void TaskObjectName::sizingValueChanged(double Value)
//{
//    pcObject->SegValue.setValue(Value);
//}
//
//void TaskObjectName::orientationToggled(bool Value)
//{
//    pcObject->UseRotation.setValue(Value);
//}
//


TaskObjectName::~TaskObjectName()
{
    delete ui;
}


#include "moc_TaskObjectName.cpp"
