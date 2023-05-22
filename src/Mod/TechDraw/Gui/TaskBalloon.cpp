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
# include <cmath>
#endif // #ifndef _PreComp_

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Control.h>

#include "TaskBalloon.h"
#include "ui_TaskBalloon.h"
#include "DrawGuiUtil.h"
#include "QGIViewBalloon.h"
#include "ViewProviderBalloon.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskBalloon::TaskBalloon(QGIViewBalloon *parent, ViewProviderBalloon *balloonVP) :
    ui(new Ui_TaskBalloon)
{
    int i = 0;
    m_parent = parent;
    m_balloonVP = balloonVP;
    m_guiDocument = balloonVP->getDocument();
    m_appDocument = parent->getBalloonFeat()->getDocument();
    m_balloonName = parent->getBalloonFeat()->getNameInDocument();

    ui->setupUi(this);

    ui->qsbShapeScale->setValue(parent->getBalloonFeat()->ShapeScale.getValue());
    connect(ui->qsbShapeScale, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskBalloon::onShapeScaleChanged);

    ui->qsbSymbolScale->setValue(parent->getBalloonFeat()->EndTypeScale.getValue());
    connect(ui->qsbSymbolScale, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskBalloon::onEndSymbolScaleChanged);

    std::string value = parent->getBalloonFeat()->Text.getValue();
    QString qs = QString::fromUtf8(value.data(), value.size());
    ui->leText->setText(qs);
    ui->leText->selectAll();
    connect(ui->leText, &QLineEdit::textChanged, this, &TaskBalloon::onTextChanged);
    QTimer::singleShot(0, ui->leText, qOverload<>(&QLineEdit::setFocus));

    DrawGuiUtil::loadArrowBox(ui->comboEndSymbol);
    i = parent->getBalloonFeat()->EndType.getValue();
    ui->comboEndSymbol->setCurrentIndex(i);
    connect(ui->comboEndSymbol, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskBalloon::onEndSymbolChanged);

    i = parent->getBalloonFeat()->BubbleShape.getValue();
    ui->comboBubbleShape->setCurrentIndex(i);
    connect(ui->comboBubbleShape, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskBalloon::onBubbleShapeChanged);

    ui->qsbFontSize->setUnit(Base::Unit::Length);
    ui->qsbFontSize->setMinimum(0);

    ui->qsbLineWidth->setUnit(Base::Unit::Length);
    ui->qsbLineWidth->setSingleStep(0.100);
    ui->qsbLineWidth->setMinimum(0);

    // negative kink length is allowed, thus no minimum
    ui->qsbKinkLength->setUnit(Base::Unit::Length);

    if (balloonVP) {
        ui->textColor->setColor(balloonVP->Color.getValue().asValue<QColor>());
        connect(ui->textColor, &ColorButton::changed, this, &TaskBalloon::onColorChanged);
        ui->qsbFontSize->setValue(balloonVP->Fontsize.getValue());
        ui->comboLineVisible->setCurrentIndex(balloonVP->LineVisible.getValue());
        ui->qsbLineWidth->setValue(balloonVP->LineWidth.getValue());
    }
    // new balloons have already the preferences BalloonKink length
    ui->qsbKinkLength->setValue(parent->getBalloonFeat()->KinkLength.getValue());

    connect(ui->qsbFontSize, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskBalloon::onFontsizeChanged);
    connect(ui->comboLineVisible, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskBalloon::onLineVisibleChanged);
    connect(ui->qsbLineWidth, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskBalloon::onLineWidthChanged);
    connect(ui->qsbKinkLength, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskBalloon::onKinkLengthChanged);

}

TaskBalloon::~TaskBalloon()
{
}

bool TaskBalloon::accept()
{
    // re issue #9626 if the balloon is deleted while the task dialog is in progress we will fail
    // trying to access the feature object or the viewprovider.  This should be prevented by
    // change to ViewProviderBalloon
    // see also reject()
    App::DocumentObject* balloonFeature = m_appDocument->getObject(m_balloonName.c_str());
    if(balloonFeature) {
        // an object with our name still exists in the document
        balloonFeature->purgeTouched();
        m_guiDocument->commitCommand();
    } else {
        // see comment in reject(). this may not do what we want.
        Gui::Command::abortCommand();
    }

    m_guiDocument->resetEdit();

    return true;
}

bool TaskBalloon::reject()
{
    // re issue #9626 - if the balloon is deleted while the dialog is in progress
    // the delete transaction is still active and ?locked? so our "abortCommand"
    // doesn't work properly and a pending transaction is still in place.  This
    // causes a warning message from App::AutoTransaction (??) that can't be
    // cleared.  Even closing the document will not clear the warning.
    // A change to ViewProviderBalloon::onDelete should prevent this from
    // happening from the Gui.  It is possible(?) that the balloon could be
    // deleted by a script.
    m_guiDocument->abortCommand();
    App::DocumentObject* balloonFeature = m_appDocument->getObject(m_balloonName.c_str());
    if(balloonFeature) {
        // an object with our name still exists in the document
        balloonFeature->recomputeFeature();
        balloonFeature->purgeTouched();
    }
    m_guiDocument->resetEdit();
    Gui::Command::updateActive();

    return true;
}

void TaskBalloon::recomputeFeature()
{
    App::DocumentObject* objVP = m_balloonVP->getObject();
    assert(objVP);
    objVP->getDocument()->recomputeFeature(objVP);
}

void TaskBalloon::onTextChanged()
{
    m_parent->getBalloonFeat()->Text.setValue(ui->leText->text().toUtf8().constData());
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
    m_parent->getBalloonFeat()->BubbleShape.setValue(ui->comboBubbleShape->currentIndex());
    recomputeFeature();
}

void TaskBalloon::onShapeScaleChanged()
{
    m_parent->getBalloonFeat()->ShapeScale.setValue(ui->qsbShapeScale->value().getValue());
    recomputeFeature();
}

void TaskBalloon::onEndSymbolChanged()
{
    m_parent->getBalloonFeat()->EndType.setValue(ui->comboEndSymbol->currentIndex());
    recomputeFeature();
}

void TaskBalloon::onEndSymbolScaleChanged()
{
    m_parent->getBalloonFeat()->EndTypeScale.setValue(ui->qsbSymbolScale->value().getValue());
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
    m_parent->getBalloonFeat()->KinkLength.setValue(ui->qsbKinkLength->value().getValue());
    recomputeFeature();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgBalloon::TaskDlgBalloon(QGIViewBalloon *parent, ViewProviderBalloon *balloonVP) :
    TaskDialog()
{
    widget  = new TaskBalloon(parent, balloonVP);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_Balloon"), widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
    setAutoCloseOnTransactionChange(true);
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
