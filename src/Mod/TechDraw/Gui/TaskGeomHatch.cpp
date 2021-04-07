/***************************************************************************
 *   Copyright (c) 2017 WandererFan <wandererfan@gmail.com>                *
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

#include <Mod/TechDraw/App/HatchLine.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "ViewProviderGeomHatch.h"
#include "TaskGeomHatch.h"
#include <Mod/TechDraw/Gui/ui_TaskGeomHatch.h>

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskGeomHatch::TaskGeomHatch(TechDraw::DrawGeomHatch* inHatch,TechDrawGui::ViewProviderGeomHatch* inVp,bool mode) :
    ui(new Ui_TaskGeomHatch),
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

TaskGeomHatch::~TaskGeomHatch()
{
}


void TaskGeomHatch::initUi()
{
    ui->fcFile->setFileName(QString::fromUtf8(m_file.data(), m_file.size()));
    std::vector<std::string> names = PATLineSpec::getPatternList(m_file);
    QStringList qsNames = listToQ(names);

    ui->cbName->addItems(qsNames);
    int nameIndex = ui->cbName->findText(QString::fromUtf8(m_name.data(),m_name.size()));
    if (nameIndex > -1) {
        ui->cbName->setCurrentIndex(nameIndex);
    } else {
        Base::Console().Warning("Warning - Pattern name *%s* not found in current PAT File\n", m_name.c_str());
    }
    connect(ui->cbName, SIGNAL(currentIndexChanged(int)), this, SLOT(onNameChanged()));

    ui->sbScale->setValue(m_scale);
    ui->sbScale->setSingleStep(0.1);
    connect(ui->sbScale, SIGNAL(valueChanged(double)), this, SLOT(onScaleChanged()));
    ui->sbWeight->setValue(m_weight);
    ui->sbWeight->setSingleStep(0.1);
    connect(ui->sbWeight, SIGNAL(valueChanged(double)), this, SLOT(onLineWeightChanged()));
    ui->ccColor->setColor(m_color.asValue<QColor>());
    connect(ui->ccColor, SIGNAL(changed()), this, SLOT(onColorChanged()));
}

//move values from screen to DocObjs
void TaskGeomHatch::updateValues()
{
    m_file = (ui->fcFile->fileName()).toUtf8().constData();
    m_hatch->FilePattern.setValue(m_file);
    QString cText = ui->cbName->currentText();
    m_name = cText.toUtf8().constData();
    m_hatch->NamePattern.setValue(m_name);
    m_scale = ui->sbScale->value().getValue();
    m_hatch->ScalePattern.setValue(m_scale);
    m_color.setValue<QColor>(ui->ccColor->color());
    m_Vp->ColorPattern.setValue(m_color);
    m_weight = ui->sbWeight->value().getValue();
    m_Vp->WeightPattern.setValue(m_weight);
}

QStringList TaskGeomHatch::listToQ(std::vector<std::string> in)
{
    QStringList result;
    for (auto& s: in) {
        QString qs = QString::fromUtf8(s.data(), s.size());
        result.append(qs);
    }
    return result;
}

void TaskGeomHatch::onFileChanged(void)
{
    m_file = ui->fcFile->fileName().toUtf8().constData();
    std::vector<std::string> names = PATLineSpec::getPatternList(m_file);
    QStringList qsNames = listToQ(names);
    ui->cbName->clear();
    ui->cbName->addItems(qsNames);
}

bool TaskGeomHatch::accept()
{
    updateValues();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    m_source->touch();
    m_source->getDocument()->recompute();          //TODO: this is only here to get graphics to update.
                                                   //      sb "redraw graphics" since m_source geom has not changed.
    return true;
}

void TaskGeomHatch::onNameChanged()
{
    QString cText = ui->cbName->currentText();
    m_name = cText.toUtf8().constData();
    m_hatch->NamePattern.setValue(m_name);
    m_source->getDocument()->recompute();
}

void TaskGeomHatch::onScaleChanged()
{
    m_hatch->ScalePattern.setValue(ui->sbScale->value().getValue());
    m_source->getDocument()->recompute();
}

void TaskGeomHatch::onLineWeightChanged()
{
    m_Vp->WeightPattern.setValue(ui->sbWeight->value().getValue());
    m_source->getDocument()->recompute();
}

void TaskGeomHatch::onColorChanged()
{
    App::Color ac;
    ac.setValue<QColor>(ui->ccColor->color());
    m_Vp->ColorPattern.setValue(ac);
    m_source->getDocument()->recompute();
}

bool TaskGeomHatch::reject()
{
    if (getCreateMode()) {
        std::string HatchName = m_hatch->getNameInDocument();
        Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().removeObject('%s')",HatchName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
        m_source->touch();
        m_source->getDocument()->recompute();
    } else {
        m_hatch->FilePattern.setValue(m_origFile);
        m_hatch->NamePattern.setValue(m_origName);
        m_hatch->ScalePattern.setValue(m_origScale);
        m_Vp->ColorPattern.setValue(m_origColor);
        m_Vp->WeightPattern.setValue(m_origWeight);
    }
    return false;
}

void TaskGeomHatch::getParameters()
{
    m_file   = m_hatch->FilePattern.getValue();
    m_name   = m_hatch->NamePattern.getValue();
    m_scale  = m_hatch->ScalePattern.getValue();
    m_color  = m_Vp->ColorPattern.getValue();
    m_weight = m_Vp->WeightPattern.getValue();
    if (!getCreateMode()) {
        m_origFile   = m_hatch->FilePattern.getValue();
        m_origName   = m_hatch->NamePattern.getValue();
        m_origScale  = m_hatch->ScalePattern.getValue();
        m_origColor  = m_Vp->ColorPattern.getValue();
        m_origWeight = m_Vp->WeightPattern.getValue();
    }
    
}

void TaskGeomHatch::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgGeomHatch::TaskDlgGeomHatch(TechDraw::DrawGeomHatch* inHatch, TechDrawGui::ViewProviderGeomHatch* inVp, bool mode) :
    TaskDialog(),
    viewProvider(nullptr)
{
    widget  = new TaskGeomHatch(inHatch,inVp, mode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_Tree_View"),
                                         widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgGeomHatch::~TaskDlgGeomHatch()
{
}

void TaskDlgGeomHatch::setCreateMode(bool b)
{
    widget->setCreateMode(b);
}

void TaskDlgGeomHatch::update()
{
    //widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgGeomHatch::open()
{
}

void TaskDlgGeomHatch::clicked(int i)
{
    Q_UNUSED(i);
}

bool TaskDlgGeomHatch::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgGeomHatch::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskGeomHatch.cpp>
