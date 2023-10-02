/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>

#include "TaskEdge2TracParameter.h"
#include "ui_TaskEdge2TracParameter.h"


using namespace RobotGui;
using namespace Gui;

TaskEdge2TracParameter::TaskEdge2TracParameter(Robot::Edge2TracObject* pcObject, QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Robot_Edge2Trac"),
              tr("TaskEdge2TracParameter"),
              true,
              parent)
    , pcObject(pcObject)
    , HideShowObj(nullptr)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskEdge2TracParameter();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    // clang-format off
    connect(ui->pushButton_HideShow, &QPushButton::clicked,
            this, &TaskEdge2TracParameter::hideShow);
    connect(ui->doubleSpinBoxSizing, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &TaskEdge2TracParameter::sizingValueChanged);
    connect(ui->checkBoxOrientation, &QCheckBox::toggled,
            this, &TaskEdge2TracParameter::orientationToggled);
    // clang-format on

    setHideShowObject();
}
void TaskEdge2TracParameter::setHideShowObject()
{
    HideShowObj = pcObject->Source.getValue();

    if (HideShowObj) {
        QString ObjectName = QString::fromUtf8(HideShowObj->Label.getValue());
        ui->lineEdit_ObjectName->setText(ObjectName);
    }
    else {
        ui->lineEdit_ObjectName->setText(QString());
    }
}

void TaskEdge2TracParameter::hideShow()
{
    setHideShowObject();

    if (HideShowObj) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc->getViewProvider(HideShowObj)->isVisible()) {
            doc->getViewProvider(HideShowObj)->setVisible(false);
        }
        else {
            doc->getViewProvider(HideShowObj)->setVisible(true);
        }
    }
}

void TaskEdge2TracParameter::sizingValueChanged(double Value)
{
    pcObject->SegValue.setValue(Value);
}

void TaskEdge2TracParameter::orientationToggled(bool Value)
{
    pcObject->UseRotation.setValue(Value);
}

void TaskEdge2TracParameter::setEdgeAndClusterNbr(int NbrEdges, int NbrClusters)
{
    QPalette palette(QApplication::palette());
    QString text;

    const int a = 200, p = 0;

    // set the text and the background color for the Edges label
    if (NbrEdges > 0) {
        palette.setBrush(QPalette::WindowText, QColor(p, a, p));
    }
    else {
        palette.setBrush(QPalette::WindowText, QColor(a, p, p));
    }

    text = QString::fromLatin1("Edges: %1").arg(NbrEdges);
    ui->label_Edges->setPalette(palette);
    ui->label_Edges->setText(text);

    // set the text and the background color for the Clusters label
    if (NbrClusters == 1) {
        palette.setBrush(QPalette::WindowText, QColor(p, a, p));
    }
    else {
        palette.setBrush(QPalette::WindowText, QColor(a, p, p));
    }

    text = QString::fromLatin1("Cluster: %1").arg(NbrClusters);
    ui->label_Cluster->setPalette(palette);
    ui->label_Cluster->setText(text);
}


TaskEdge2TracParameter::~TaskEdge2TracParameter()
{
    delete ui;
}


#include "moc_TaskEdge2TracParameter.cpp"
