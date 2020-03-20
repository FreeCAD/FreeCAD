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
#include <Base/Vector3D.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
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

    ui->inputScale->setValue(parent->dvBalloon->ShapeScale.getValue());

    std::string value = parent->dvBalloon->Text.getValue();
    QString qs = QString::fromUtf8(value.data(), value.size());
    ui->inputValue->setText(qs);
    ui->inputValue->selectAll();
    QTimer::singleShot(0, ui->inputValue, SLOT(setFocus()));

    DrawGuiUtil::loadArrowBox(ui->comboEndType);
    i = parent->dvBalloon->EndType.getValue();
    ui->comboEndType->setCurrentIndex(i);

    i = parent->dvBalloon->Shape.getValue();
    ui->comboSymbol->setCurrentIndex(i);

    if (m_balloonVP != nullptr)
        ui->textColor->setColor(m_balloonVP->Color.getValue().asValue<QColor>());

    ui->qsbFontSize->setUnit(Base::Unit::Length);
    ui->qsbLineWidth->setUnit(Base::Unit::Length);
    ui->qsbLineWidth->setSingleStep(0.100);
    ui->qsbBalloonKink->setUnit(Base::Unit::Length);

    ui->qsbFontSize->setValue(m_balloonVP->Fontsize.getValue());
    ui->qsbLineWidth->setValue(m_balloonVP->LineWidth.getValue());
    // new balloons have already the preferences BalloonKink length
    ui->qsbBalloonKink->setValue(m_parent->dvBalloon->KinkLength.getValue());
}

TaskBalloon::~TaskBalloon()
{
    delete ui;
}

bool TaskBalloon::accept()
{
        m_parent->dvBalloon->Text.setValue(ui->inputValue->text().toUtf8().constData());
        App::Color ac;
        ac.setValue<QColor>(ui->textColor->color());
        m_balloonVP->Color.setValue(ac);
        m_balloonVP->Fontsize.setValue(ui->qsbFontSize->value().getValue());
        m_parent->dvBalloon->ShapeScale.setValue(ui->inputScale->value());
        m_parent->dvBalloon->EndType.setValue(ui->comboEndType->currentIndex());
        m_parent->dvBalloon->Shape.setValue(ui->comboSymbol->currentIndex());
        m_balloonVP->LineWidth.setValue(ui->qsbLineWidth->value().getValue());
        m_parent->dvBalloon->KinkLength.setValue(ui->qsbBalloonKink->value().getValue());
        m_parent->updateView(true);

    return true;
}

bool TaskBalloon::reject()
{
    return false;
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
