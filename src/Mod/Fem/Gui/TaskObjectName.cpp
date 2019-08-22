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
    : TaskBox(Gui::BitmapFactory().pixmap("fem-femmesh-create-node-by-poly"),
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

    QObject::connect(ui->lineEdit_ObjectName,SIGNAL(textChanged (const QString&)),this,SLOT(TextChanged(const QString&)));

    if(strcmp(pcObject->Label.getValue(),"") != 0)
        ui->lineEdit_ObjectName->setText(QString::fromUtf8(pcObject->Label.getValue()));
    else
        ui->lineEdit_ObjectName->setText(QString::fromLatin1(pcObject->getNameInDocument()));

}


void TaskObjectName::TextChanged (const QString & text)
{
    name = text.toUtf8().constData();
    //pcObject->Label.setValue(text.toUtf8());
}



TaskObjectName::~TaskObjectName()
{
    delete ui;
}


#include "moc_TaskObjectName.cpp"
