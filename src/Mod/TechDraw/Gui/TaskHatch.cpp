/***************************************************************************
 *   Copyright (c) 2020 FreeCAD Developers                                 *
 *   Author: Uwe Stöhr <uwestoehr@lyx.org>                                 *
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

#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "ViewProviderHatch.h"
#include "TaskHatch.h"
#include <Mod/TechDraw/Gui/ui_TaskHatch.h>

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskHatch::TaskHatch(TechDraw::DrawHatch* inHatch, TechDrawGui::ViewProviderHatch* inVp, bool mode) :
    ui(new Ui_TaskHatch),
    m_hatch(inHatch),
    m_Vp(inVp),
    m_createMode(mode)
{
    ui->setupUi(this);
    connect(ui->fcFile, SIGNAL(fileNameSelected( const QString & )), this, SLOT(onFileChanged(void)));

    m_source = m_hatch->Source.getValue();
    getParameters();
    initUi();
}

TaskHatch::~TaskHatch()
{
}

void TaskHatch::initUi()
{
    ui->fcFile->setFileName(QString::fromUtf8(m_file.data(), m_file.size()));
    ui->sbScale->setValue(m_scale);
    ui->sbScale->setSingleStep(0.1);
    connect(ui->sbScale, SIGNAL(valueChanged(double)), this, SLOT(onScaleChanged()));
    ui->ccColor->setColor(m_color.asValue<QColor>());
    connect(ui->ccColor, SIGNAL(changed()), this, SLOT(onColorChanged()));
}

//move values from screen to DocObjs
void TaskHatch::updateValues()
{
    m_file = (ui->fcFile->fileName()).toUtf8().constData();
    m_hatch->HatchPattern.setValue(m_file);
    m_scale = ui->sbScale->value().getValue();
    m_Vp->HatchScale.setValue(m_scale);
    m_color.setValue<QColor>(ui->ccColor->color());
    m_Vp->HatchColor.setValue(m_color);
}

QStringList TaskHatch::listToQ(std::vector<std::string> in)
{
    QStringList result;
    for (auto& s: in) {
        QString qs = QString::fromUtf8(s.data(), s.size());
        result.append(qs);
    }
    return result;
}

void TaskHatch::onFileChanged(void)
{
    m_file = ui->fcFile->fileName().toUtf8().constData();
    m_hatch->HatchPattern.setValue(m_file);
    m_source->getDocument()->recompute();
}

bool TaskHatch::accept()
{
    updateValues();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    m_source->touch();
    m_source->getDocument()->recompute();
    return true;
}

void TaskHatch::onScaleChanged()
{
    m_Vp->HatchScale.setValue(ui->sbScale->value().getValue());
    m_source->getDocument()->recompute();
}

void TaskHatch::onColorChanged()
{
    App::Color ac;
    ac.setValue<QColor>(ui->ccColor->color());
    m_Vp->HatchColor.setValue(ac);
    m_source->getDocument()->recompute();
}

bool TaskHatch::reject()
{
    if (getCreateMode()) {
        std::string HatchName = m_hatch->getNameInDocument();
        Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().removeObject('%s')",HatchName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
        m_source->touch();
        m_source->getDocument()->recompute();
    } else {
        m_hatch->HatchPattern.setValue(m_origFile);
        m_Vp->HatchScale.setValue(m_origScale);
        m_Vp->HatchColor.setValue(m_origColor);
    }
    return false;
}

void TaskHatch::getParameters()
{
    m_file = m_hatch->HatchPattern.getValue();
    m_scale = m_Vp->HatchScale.getValue();
    m_color = m_Vp->HatchColor.getValue();
    if (!getCreateMode()) {
        m_origFile = m_hatch->HatchPattern.getValue();
        m_origScale = m_Vp->HatchScale.getValue();
        m_origColor = m_Vp->HatchColor.getValue();
    }
}

void TaskHatch::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgHatch::TaskDlgHatch(TechDraw::DrawHatch* inHatch, TechDrawGui::ViewProviderHatch* inVp, bool mode) :
    TaskDialog(),
    viewProvider(nullptr)
{
    widget  = new TaskHatch(inHatch, inVp, mode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_Tree_View"),
                                         widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgHatch::~TaskDlgHatch()
{
}

void TaskDlgHatch::setCreateMode(bool b)
{
    widget->setCreateMode(b);
}

void TaskDlgHatch::update()
{
    //widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgHatch::open()
{
}

void TaskDlgHatch::clicked(int i)
{
    Q_UNUSED(i);
}

bool TaskDlgHatch::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgHatch::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskHatch.cpp>
