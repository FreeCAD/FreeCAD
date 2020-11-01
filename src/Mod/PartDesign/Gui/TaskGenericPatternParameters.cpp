/***************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder)<realthunder.dev@gmail.com>*
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
# include <QMessageBox>
# include <QAction>
# include <QTimer>
#endif

#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>

#include <Mod/PartDesign/App/FeatureGenericPattern.h>

#include "TaskMultiTransformParameters.h"
#include "Utils.h"

#include "ui_TaskGenericPatternParameters.h"
#include "TaskGenericPatternParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskGenericPatternParameters */

TaskGenericPatternParameters::TaskGenericPatternParameters(ViewProviderTransformed *TransformedView,QWidget *parent)
        : TaskTransformedParameters(TransformedView, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskGenericPatternParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskGenericPatternParameters::TaskGenericPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask)
{
    proxy = new QWidget(parentTask);
    ui = new Ui_TaskGenericPatternParameters();
    ui->setupUi(proxy);
    connect(ui->buttonOK, SIGNAL(clicked(bool)),
            parentTask, SLOT(onSubTaskButtonOK()));
    QMetaObject::connectSlotsByName(this);

    layout->addWidget(proxy);

    ui->buttonOK->setEnabled(true);
    ui->checkBoxUpdateView->hide();

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskGenericPatternParameters::setupUI()
{
    TaskTransformedParameters::setupUI();

    connect(ui->editExpression, SIGNAL(textChanged()), this, SLOT(onChangedExpression()));

    updateUI();

    ui->editExpression->setDocumentObject(getObject());
}

void TaskGenericPatternParameters::updateUI()
{
    Base::StateLocker lock(blockUpdate);

    PartDesign::GenericPattern* pcGenericPattern = static_cast<PartDesign::GenericPattern*>(getObject());
    if (!pcGenericPattern)
        return;
    App::ObjectIdentifier path(pcGenericPattern->MatrixList);
    auto info = pcGenericPattern->getExpression(path);
    if (!info.expression) {
        this->expr.reset();
        ui->editExpression->setPlainText(QString());
    }
    else {
        ui->editExpression->setPlainText(QString::fromUtf8(info.expression->toString().c_str()));
        this->expr = info.expression->copy();
    }
}

void TaskGenericPatternParameters::onChangedExpression() {
    if (blockUpdate)
        return;

    auto obj = Base::freecad_dynamic_cast<PartDesign::GenericPattern>(getObject());
    if (!obj)
        return;

    if (ui->editExpression->toPlainText().isEmpty()) {
        expr.reset();
        ui->labelExpressionMessage->setText(QString());
        return;
    }

    try {
        expr = App::Expression::parse(obj, ui->editExpression->toPlainText().toUtf8().constData());
        ui->labelExpressionMessage->setText(QString());
        App::ObjectIdentifier path(obj->MatrixList);
        obj->setExpression(path, expr);
        kickUpdateViewTimer();
    } catch (Base::Exception & e) {
        ui->labelExpressionMessage->setText(
                QString::fromLatin1("<font color='red'>%1</font>").arg(
                    QObject::tr(e.what())));
    }
}

void TaskGenericPatternParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on && (expr || ui->editExpression->toPlainText().isEmpty()))
        apply();
}

TaskGenericPatternParameters::~TaskGenericPatternParameters()
{
    delete ui;
    if (proxy)
        delete proxy;
}

void TaskGenericPatternParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskGenericPatternParameters::apply()
{
    auto obj = Base::freecad_dynamic_cast<PartDesign::GenericPattern>(getObject());
    if (!obj)
        return;
    setupTransaction();
    try {
        App::ObjectIdentifier path(obj->MatrixList);
        obj->setExpression(path, expr);
        if (!blockUpdate)
            recomputeFeature();
    }
    catch(Base::Exception &e) {
        e.ReportException();
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgGenericPatternParameters::TaskDlgGenericPatternParameters(ViewProviderGenericPattern *GenericPatternView)
    : TaskDlgTransformedParameters(GenericPatternView, new TaskGenericPatternParameters(GenericPatternView))
{
}

#include "moc_TaskGenericPatternParameters.cpp"
