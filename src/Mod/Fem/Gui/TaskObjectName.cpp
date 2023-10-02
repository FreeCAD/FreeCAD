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

#include <App/DocumentObject.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>

#include "TaskObjectName.h"
#include "ui_TaskObjectName.h"


using namespace FemGui;
using namespace Gui;

TaskObjectName::TaskObjectName(App::DocumentObject* pcObject, QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap("FEM_CreateNodesSet"), tr("TaskObjectName"), true, parent)
    , pcObject(pcObject)
    , ui(new Ui_TaskObjectName)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    QObject::connect(ui->lineEdit_ObjectName,
                     &QLineEdit::textChanged,
                     this,
                     &TaskObjectName::TextChanged);

    if (strcmp(pcObject->Label.getValue(), "") != 0) {
        ui->lineEdit_ObjectName->setText(QString::fromUtf8(pcObject->Label.getValue()));
    }
    else {
        ui->lineEdit_ObjectName->setText(QString::fromLatin1(pcObject->getNameInDocument()));
    }
}

void TaskObjectName::TextChanged(const QString& text)
{
    name = text.toUtf8().constData();
    // pcObject->Label.setValue(text.toUtf8());
}

TaskObjectName::~TaskObjectName() = default;


#include "moc_TaskObjectName.cpp"
