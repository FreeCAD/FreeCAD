/***************************************************************************
 *   Copyright (c) 2018 WandererFan <wandererfan@gmail.com>                *
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
#include <Base/Tools.h>
#include <Base/Vector3D.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/FileDialog.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>

#include "TaskLineDecor.h"
#include <Mod/TechDraw/Gui/ui_TaskLineDecor.h>

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskLineDecor::TaskLineDecor(TechDraw::DrawViewPart* partFeat,
                             std::vector<std::string> edgeNames) :
    ui(new Ui_TaskLineDecor),
    m_partFeat(partFeat),
    m_edges(edgeNames)
{
    getDefaults();
    ui->setupUi(this);

    connect(ui->cb_Style, SIGNAL(currentIndexChanged( int )), this, SLOT(onStyleChanged(void)));
    connect(ui->cc_Color, SIGNAL(changed(  )), this, SLOT(onColorChanged(void)));
    connect(ui->dsb_Weight, SIGNAL(valueChanged( double  )), this, SLOT(onWeightChanged( void )));
    connect(ui->cb_Visible, SIGNAL(currentIndexChanged( int )), this, SLOT(onVisibleChanged( void )));

    initUi();
}

TaskLineDecor::~TaskLineDecor()
{
    delete ui;
}

void TaskLineDecor::initUi()
{
    std::string viewName = m_partFeat->getNameInDocument();
    ui->le_View->setText(Base::Tools::fromStdString(viewName));

    std::stringstream ss;
    for (auto& e: m_edges) {
        int num = DrawUtil::getIndexFromName(e);
        ss << num << ", ";
    }
    std::string temp = ss.str();
    if (!temp.empty()) {
        temp.pop_back();
    }
    ui->le_Lines->setText(Base::Tools::fromStdString(temp));

    ui->cb_Style->setCurrentIndex(m_style);
    ui->cc_Color->setColor(m_color.asValue<QColor>());
    ui->dsb_Weight->setValue(m_weight);
    ui->cb_Visible->setCurrentIndex(m_visible);
}

void TaskLineDecor::getDefaults(void) 
{
    m_style = LineFormat::getDefEdgeStyle();
    m_color = LineFormat::getDefEdgeColor();
    m_weight = LineFormat::getDefEdgeWidth();
    m_visible = 1;

    //set defaults to format of 1st edge
    int num = DrawUtil::getIndexFromName(m_edges.front());
    BaseGeom* bg = m_partFeat->getGeomByIndex(num);
    if (bg != nullptr) {
        if (bg->cosmetic) {
            if (bg->source() == 1) {
                TechDraw::CosmeticEdge* ce = m_partFeat->getCosmeticEdgeByIndex(bg->sourceIndex());
                m_style = ce->m_format.m_style;
                m_color = ce->m_format.m_color;
                m_weight = ce->m_format.m_weight;
                m_visible = ce->m_format.m_visible;
            } else if (bg->source() == 2) {
                TechDraw::CenterLine* cl = m_partFeat->getCenterLineByIndex(bg->sourceIndex());
                m_style = cl->m_format.m_style;
                m_color = cl->m_format.m_color;
                m_weight = cl->m_format.m_weight;
                m_visible = cl->m_format.m_visible;
            }
        } else {
                TechDraw::GeomFormat* gf = m_partFeat->getGeomFormatByGeom(num);
                if (gf != nullptr) {
                    m_style = gf->m_format.m_style;
                    m_color = gf->m_format.m_color;
                    m_weight = gf->m_format.m_weight;
                    m_visible = gf->m_format.m_visible;
                }
        }
    }
}

void TaskLineDecor::onStyleChanged(void)
{
    m_style = ui->cb_Style->currentIndex();
    //livePreview(); 
}

void TaskLineDecor::onColorChanged(void)
{
    m_color.setValue<QColor>(ui->cc_Color->color());
    //livePreview(); 
}

void TaskLineDecor::onWeightChanged(void)
{
    m_weight = ui->dsb_Weight->value();
    //livePreview();
}

void TaskLineDecor::onVisibleChanged(void)
{
    m_visible = ui->cb_Visible->currentIndex();
    //livePreview();
}

void TaskLineDecor::applyDecorations(void)
{
//    Base::Console().Message("TLD::applyDecorations()\n");
    for (auto& e: m_edges) {
        int num = DrawUtil::getIndexFromName(e);
        BaseGeom* bg = m_partFeat->getGeomByIndex(num);
        if (bg != nullptr) {
            if (bg->cosmetic) {
                if (bg->source() == 1) {
                    TechDraw::CosmeticEdge* ce = m_partFeat->getCosmeticEdgeByIndex(bg->sourceIndex());
                    ce->m_format.m_style = m_style;
                    ce->m_format.m_color = m_color;
                    ce->m_format.m_weight = m_weight;
                    ce->m_format.m_visible = m_visible;
                } else if (bg->source() == 2) {
                    TechDraw::CenterLine* cl = m_partFeat->getCenterLineByIndex(bg->sourceIndex());
                    cl->m_format.m_style = m_style;
                    cl->m_format.m_color = m_color;
                    cl->m_format.m_weight = m_weight;
                    cl->m_format.m_visible = m_visible;
                }
            } else {
                TechDraw::GeomFormat* gf = m_partFeat->getGeomFormatByGeom(num);
                if (gf != nullptr) {
                    gf->m_format.m_style = m_style;
                    gf->m_format.m_color = m_color;
                    gf->m_format.m_weight = m_weight;
                    gf->m_format.m_visible = m_visible;
                } else {
                    TechDraw::LineFormat fmt(m_style,
                                             m_weight,
                                             m_color,
                                             m_visible);
                    TechDraw::GeomFormat* newGF = new TechDraw::GeomFormat(num,
                                                                           fmt);
//                    int idx = 
                    m_partFeat->addGeomFormat(newGF);
               }
            }
        }
    }
}

bool TaskLineDecor::accept()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_partFeat->getDocument());
    if (!doc) return false;

    applyDecorations();
    m_partFeat->requestPaint();

    //Gui::Command::updateActive();     //no chain of updates here
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskLineDecor::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_partFeat->getDocument());
    if (!doc) return false;

    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    return false;
}

void TaskLineDecor::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgLineDecor::TaskDlgLineDecor(TechDraw::DrawViewPart* partFeat,
                                   std::vector<std::string> edgeNames) :
    TaskDialog()
{
    widget  = new TaskLineDecor(partFeat, edgeNames);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-linedecor"),
                                         widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgLineDecor::~TaskDlgLineDecor()
{
}

//==== calls from the TaskView ===============================================================
void TaskDlgLineDecor::open()
{

}

void TaskDlgLineDecor::clicked(int i)
{
    Q_UNUSED(i);
}

bool TaskDlgLineDecor::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgLineDecor::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskLineDecor.cpp>
