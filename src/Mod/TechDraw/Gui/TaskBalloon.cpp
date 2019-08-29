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

#include <Mod/TechDraw/Gui/ui_TaskBalloon.h>

#include "QGIViewBalloon.h"
#include "TaskBalloon.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskBalloon::TaskBalloon(QGIViewBalloon *parent) :
    ui(new Ui_TaskBalloon)
{
    int i = 0;

    m_parent = parent;

    ui->setupUi(this);

    QString qs = QString::number(parent->dvBalloon->SymbolScale.getValue(), 'f', 2);
    ui->inputScale->setText(qs);

    std::string value = parent->dvBalloon->Text.getValue();
    qs = QString::fromUtf8(value.data(), value.size());
    ui->inputValue->setText(qs);
    ui->inputValue->selectAll();
    QTimer::singleShot(0, ui->inputValue, SLOT(setFocus()));

    i = parent->dvBalloon->EndType.getValue();
    ui->comboEndType->setCurrentIndex(i);

    i = parent->dvBalloon->Symbol.getValue();
    ui->comboSymbol->setCurrentIndex(i);

}

TaskBalloon::~TaskBalloon()
{
    delete ui;
}


bool TaskBalloon::accept()
{
        m_parent->dvBalloon->Text.setValue(ui->inputValue->text().toUtf8().constData());
        m_parent->dvBalloon->SymbolScale.setValue(ui->inputScale->text().toDouble());
        m_parent->dvBalloon->EndType.setValue(ui->comboEndType->currentIndex());
        m_parent->dvBalloon->Symbol.setValue(ui->comboSymbol->currentIndex());
        m_parent->updateView(true);

    return true;
}

bool TaskBalloon::reject()
{
    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgBalloon::TaskDlgBalloon(QGIViewBalloon *parent) :
    TaskDialog()
{
    widget  = new TaskBalloon(parent);
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
