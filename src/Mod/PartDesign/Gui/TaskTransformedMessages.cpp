/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "ui_TaskTransformedMessages.h"
#include "TaskTransformedMessages.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>

#include <boost/bind.hpp>

#include "ViewProviderTransformed.h"

using namespace PartDesignGui;
using namespace Gui::TaskView;

TaskTransformedMessages::TaskTransformedMessages(ViewProviderTransformed *transformedView_)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Transformed feature messages"),true, 0),
      transformedView(transformedView_)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskTransformedMessages();
    ui->setupUi(proxy);
    // set a minimum height to avoid a sudden resize and to
    // lose focus of the currently used spin boxes
    ui->labelTransformationStatus->setMinimumHeight(50);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    connectionDiagnosis = transformedView->signalDiagnosis.connect(boost::bind(&PartDesignGui::TaskTransformedMessages::slotDiagnosis, this,_1));
}

TaskTransformedMessages::~TaskTransformedMessages()
{
    connectionDiagnosis.disconnect();
    delete ui;
}

void TaskTransformedMessages::slotDiagnosis(QString msg)
{
    ui->labelTransformationStatus->setText(msg);
}

QCheckBox *TaskTransformedMessages::getCheckBox() {
    return ui->checkBoxSubTransform;
}

#include "moc_TaskTransformedMessages.cpp"
