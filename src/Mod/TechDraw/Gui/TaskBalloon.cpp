/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
 *   Copyright (c) 2019 Franck Jullien <franck.jullien@gmail.com>          *
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
#include <cmath>
#endif // #ifndef _PreComp_

#include <Base/Console.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/ArrowPropEnum.h>

#include <Mod/TechDraw/Gui/ui_TaskBalloon.h>

#include "DrawGuiUtil.h"
#include "QGIViewBalloon.h"
#include "ViewProviderBalloon.h"
#include "TaskBalloon.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskBalloon::TaskBalloon(QGIViewBalloon *parent, ViewProviderBalloon *balloonVP) :
    ui(new Ui_TaskBalloon)
{
    int i = 0;
    m_parent = parent;
    m_balloonVP = balloonVP;

    ui->setupUi(this);

    ui->qsbShapeScale->setValue(parent->dvBalloon->ShapeScale.getValue());
    connect(ui->qsbShapeScale, SIGNAL(valueChanged(double)), this, SLOT(onShapeScaleChanged()));

    ui->qsbSymbolScale->setValue(parent->dvBalloon->EndTypeScale.getValue());
    connect(ui->qsbSymbolScale, SIGNAL(valueChanged(double)), this, SLOT(onEndSymbolScaleChanged()));

    std::string value = parent->dvBalloon->Text.getValue();
    QString qs = QString::fromUtf8(value.data(), value.size());
    ui->leText->setText(qs);
    ui->leText->selectAll();
    connect(ui->leText, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged()));
    QTimer::singleShot(0, ui->leText, SLOT(setFocus()));

    DrawGuiUtil::loadArrowBox(ui->comboEndSymbol);
    i = parent->dvBalloon->EndType.getValue();
    ui->comboEndSymbol->setCurrentIndex(i);
    connect(ui->comboEndSymbol, SIGNAL(currentIndexChanged(int)), this, SLOT(onEndSymbolChanged()));

    i = parent->dvBalloon->BubbleShape.getValue();
    ui->comboBubbleShape->setCurrentIndex(i);
    connect(ui->comboBubbleShape, SIGNAL(currentIndexChanged(int)), this, SLOT(onBubbleShapeChanged()));

    ui->qsbFontSize->setUnit(Base::Unit::Length);
    ui->qsbFontSize->setMinimum(0);
    connect(ui->qsbFontSize, SIGNAL(valueChanged(double)), this, SLOT(onFontsizeChanged()));
    connect(ui->comboLineVisible, SIGNAL(currentIndexChanged(int)), this, SLOT(onLineVisibleChanged()));
    ui->qsbLineWidth->setUnit(Base::Unit::Length);
    ui->qsbLineWidth->setSingleStep(0.100);
    ui->qsbLineWidth->setMinimum(0);
    connect(ui->qsbLineWidth, SIGNAL(valueChanged(double)), this, SLOT(onLineWidthChanged()));
    ui->qsbKinkLength->setUnit(Base::Unit::Length);
    // negative kink length is allowed, thus no minimum
    connect(ui->qsbKinkLength, SIGNAL(valueChanged(double)), this, SLOT(onKinkLengthChanged()));

    if (balloonVP != nullptr) {
        ui->textColor->setColor(balloonVP->Color.getValue().asValue<QColor>());
        connect(ui->textColor, SIGNAL(changed()), this, SLOT(onColorChanged()));
        ui->qsbFontSize->setValue(balloonVP->Fontsize.getValue());
        ui->comboLineVisible->setCurrentIndex(balloonVP->LineVisible.getValue());
        ui->qsbLineWidth->setValue(balloonVP->LineWidth.getValue());
    }
    // new balloons have already the preferences BalloonKink length
    ui->qsbKinkLength->setValue(parent->dvBalloon->KinkLength.getValue());
}

TaskBalloon::~TaskBalloon()
{
}

bool TaskBalloon::accept()
{
    m_parent->dvBalloon->Text.setValue(ui->leText->text().toUtf8().constData());
    App::Color ac;
    ac.setValue<QColor>(ui->textColor->color());
    m_balloonVP->Color.setValue(ac);
    m_balloonVP->Fontsize.setValue(ui->qsbFontSize->value().getValue());
    m_parent->dvBalloon->ShapeScale.setValue(ui->qsbShapeScale->value().getValue());
    m_parent->dvBalloon->EndType.setValue(ui->comboEndSymbol->currentIndex());
    m_parent->dvBalloon->EndTypeScale.setValue(ui->qsbSymbolScale->value().getValue());
    m_parent->dvBalloon->BubbleShape.setValue(ui->comboBubbleShape->currentIndex());
    m_balloonVP->LineVisible.setValue(ui->comboLineVisible->currentIndex());
    m_balloonVP->LineWidth.setValue(ui->qsbLineWidth->value().getValue());
    m_parent->dvBalloon->KinkLength.setValue(ui->qsbKinkLength->value().getValue());
    m_parent->updateView(true);

    return true;
}

bool TaskBalloon::reject()
{
    return false;
}

void TaskBalloon::recomputeFeature()
{
    App::DocumentObject* objVP = m_balloonVP->getObject();
    assert(objVP);
    objVP->getDocument()->recomputeFeature(objVP);
}

void TaskBalloon::onTextChanged()
{
    m_parent->dvBalloon->Text.setValue(ui->leText->text().toUtf8().constData());
    recomputeFeature();
}

void TaskBalloon::onColorChanged()
{
    App::Color ac;
    ac.setValue<QColor>(ui->textColor->color());
    m_balloonVP->Color.setValue(ac);
    recomputeFeature();
}

void TaskBalloon::onFontsizeChanged()
{
    m_balloonVP->Fontsize.setValue(ui->qsbFontSize->value().getValue());
    recomputeFeature();
}

void TaskBalloon::onBubbleShapeChanged()
{
    m_parent->dvBalloon->BubbleShape.setValue(ui->comboBubbleShape->currentIndex());
    recomputeFeature();
}

void TaskBalloon::onShapeScaleChanged()
{
    m_parent->dvBalloon->ShapeScale.setValue(ui->qsbShapeScale->value().getValue());
    recomputeFeature();
}

void TaskBalloon::onEndSymbolChanged()
{
    m_parent->dvBalloon->EndType.setValue(ui->comboEndSymbol->currentIndex());
    recomputeFeature();
}

void TaskBalloon::onEndSymbolScaleChanged()
{
    m_parent->dvBalloon->EndTypeScale.setValue(ui->qsbSymbolScale->value().getValue());
    recomputeFeature();
}

void TaskBalloon::onLineVisibleChanged()
{
    m_balloonVP->LineVisible.setValue(ui->comboLineVisible->currentIndex());
    recomputeFeature();
}

void TaskBalloon::onLineWidthChanged()
{
    m_balloonVP->LineWidth.setValue(ui->qsbLineWidth->value().getValue());
    recomputeFeature();
}

void TaskBalloon::onKinkLengthChanged()
{
    m_parent->dvBalloon->KinkLength.setValue(ui->qsbKinkLength->value().getValue());
    recomputeFeature();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgBalloon::TaskDlgBalloon(QGIViewBalloon *parent, ViewProviderBalloon *balloonVP) :
    TaskDialog()
{
    widget  = new TaskBalloon(parent, balloonVP);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_Balloon"), widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgBalloon::~TaskDlgBalloon()
{
}

void TaskDlgBalloon::update()
{
    //widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgBalloon::open()
{
}

void TaskDlgBalloon::clicked(int i)
{
    Q_UNUSED(i);
}

bool TaskDlgBalloon::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgBalloon::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskBalloon.cpp>
