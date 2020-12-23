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

#include <Mod/TechDraw/Gui/ui_TaskLineDecor.h>
#include <Mod/TechDraw/Gui/ui_TaskRestoreLines.h>

#include "QGIView.h"
#include "ViewProviderViewPart.h"
#include "TaskLineDecor.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskLineDecor::TaskLineDecor(TechDraw::DrawViewPart* partFeat,
                             std::vector<std::string> edgeNames) :
    ui(new Ui_TaskLineDecor),
    m_partFeat(partFeat),
    m_edges(edgeNames),
    m_apply(true)
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

    ui->cb_Style->setCurrentIndex(m_style - 1);          //combobox does not have 0:NoLine choice
    ui->cc_Color->setColor(m_color.asValue<QColor>());
    ui->dsb_Weight->setValue(m_weight);
    ui->cb_Visible->setCurrentIndex(m_visible);
}

void TaskLineDecor::getDefaults(void) 
{
//    Base::Console().Message("TLD::getDefaults()\n");
    m_style = LineFormat::getDefEdgeStyle();
    m_color = LineFormat::getDefEdgeColor();
    m_weight = LineFormat::getDefEdgeWidth();
    m_visible = 1;

    //set defaults to format of 1st edge
    if (!m_edges.empty()) {
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
                } else {
                    Gui::ViewProvider* vp = QGIView::getViewProvider(m_partFeat);
                    auto partVP = dynamic_cast<ViewProviderViewPart*>(vp);
                    if ( vp != nullptr ) {
                        m_weight = partVP->LineWidth.getValue();
                        m_style = Qt::SolidLine;                  // = 1
                        m_color = LineFormat::getDefEdgeColor();
                        m_visible = 1;
                    }
                }
            }
        }
    }
}

void TaskLineDecor::onStyleChanged(void)
{
    m_style = ui->cb_Style->currentIndex() + 1;
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
//    Base::Console().Message("TLD::accept()\n");
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_partFeat->getDocument());
    if (!doc) return false;

    if (apply()) {
        applyDecorations();
    }

    m_partFeat->requestPaint();

    //Gui::Command::updateActive();     //no chain of updates here
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskLineDecor::reject()
{
//    Base::Console().Message("TLD::reject()\n");
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskRestoreLines::TaskRestoreLines(TechDraw::DrawViewPart* partFeat,
                                   TaskLineDecor* parent) :
    ui(new Ui_TaskRestoreLines),
    m_partFeat(partFeat),
    m_parent(parent)
{
    ui->setupUi(this);

    connect(ui->pb_All, SIGNAL(clicked( )), this, SLOT(onAllPressed(void)));
    connect(ui->pb_Geometry, SIGNAL(clicked(  )), this, SLOT(onGeometryPressed(void)));
    connect(ui->pb_Cosmetic, SIGNAL(clicked(   )), this, SLOT(onCosmeticPressed( void )));
    connect(ui->pb_Center, SIGNAL(clicked(  )), this, SLOT(onCenterPressed( void )));

    initUi();
}

TaskRestoreLines::~TaskRestoreLines()
{
    delete ui;
}

void TaskRestoreLines::initUi()
{
    ui->l_All->setText(QString::number(countInvisibleLines()));
    ui->l_Geometry->setText(QString::number(countInvisibleGeoms()));
    ui->l_Cosmetic->setText(QString::number(countInvisibleCosmetics()));
    ui->l_Center->setText(QString::number(countInvisibleCenters()));
}

void TaskRestoreLines::onAllPressed(void)
{
//    Base::Console().Message("TRL::onAllPressed()\n");
    onGeometryPressed();
    onCosmeticPressed();
    onCenterPressed();
}

void TaskRestoreLines::onGeometryPressed(void)
{
//    Base::Console().Message("TRL::onGeometryPressed()\n");
    restoreInvisibleGeoms();
    ui->l_Geometry->setText(QString::number(0));
    ui->l_All->setText(QString::number(countInvisibleLines()));
}

void TaskRestoreLines::onCosmeticPressed(void)
{
//    Base::Console().Message("TRL::onCosmeticPressed()\n");
    restoreInvisibleCosmetics();
    ui->l_Cosmetic->setText(QString::number(0));
    ui->l_All->setText(QString::number(countInvisibleLines()));
}

void TaskRestoreLines::onCenterPressed(void)
{
//    Base::Console().Message("TRL::onCenterPressed()\n");
    restoreInvisibleCenters();
    ui->l_Center->setText(QString::number(0));
    ui->l_All->setText(QString::number(countInvisibleLines()));
}

int TaskRestoreLines::countInvisibleLines(void)
{
    int result = 0;
    result += countInvisibleGeoms();
    result += countInvisibleCosmetics();
    result += countInvisibleCenters();
    return result;
}

int TaskRestoreLines::countInvisibleGeoms(void)
{
    int iGeoms = 0;
    const std::vector<TechDraw::GeomFormat*> geoms = m_partFeat->GeomFormats.getValues();
    for (auto& g : geoms) {
        if (!g->m_format.m_visible) {
            iGeoms++;
        }
    }
    return iGeoms;
}

int TaskRestoreLines::countInvisibleCosmetics(void)
{
    int iCosmos = 0;
    const std::vector<TechDraw::CosmeticEdge*> cosmos = m_partFeat->CosmeticEdges.getValues();
    for (auto& c : cosmos) {
        if (!c->m_format.m_visible) { 
            iCosmos++;
        }
    }
    return iCosmos++;
}

int TaskRestoreLines::countInvisibleCenters(void)
{
    int iCenter = 0;
    const std::vector<TechDraw::CenterLine*> centers = m_partFeat->CenterLines.getValues();
    for (auto& c : centers) {
        if (!c->m_format.m_visible) { 
            iCenter++;
        }
    }
    return iCenter++;
}

void TaskRestoreLines::restoreInvisibleLines(void)
{
    restoreInvisibleGeoms();
    restoreInvisibleCosmetics();
    restoreInvisibleCenters();
}

void TaskRestoreLines::restoreInvisibleGeoms(void)
{
    const std::vector<TechDraw::GeomFormat*> geoms = m_partFeat->GeomFormats.getValues();
    for (auto& g : geoms) {
        if (!g->m_format.m_visible) {
            g->m_format.m_visible = true;
        }
    }
    m_partFeat->GeomFormats.setValues(geoms);
    m_parent->apply(false);                   //don't undo the work we just did
}

void TaskRestoreLines::restoreInvisibleCosmetics(void)
{
    const std::vector<TechDraw::CosmeticEdge*> cosmos = m_partFeat->CosmeticEdges.getValues();
    for (auto& c : cosmos) {
        if (!c->m_format.m_visible) { 
            c->m_format.m_visible = true;
        }
    }
    m_partFeat->CosmeticEdges.setValues(cosmos);
    m_parent->apply(false);                   //don't undo the work we just did
}

void TaskRestoreLines::restoreInvisibleCenters(void)
{
    const std::vector<TechDraw::CenterLine*> centers = m_partFeat->CenterLines.getValues();
    for (auto& c : centers) {
        if (!c->m_format.m_visible) { 
            c->m_format.m_visible = true;
        }
    }
    m_partFeat->CenterLines.setValues(centers);
    m_parent->apply(false);                   //don't undo the work we just did
}


bool TaskRestoreLines::accept()
{
//    Base::Console().Message("TRL::accept()\n");
    return true;
}

bool TaskRestoreLines::reject()
{
//    Base::Console().Message("TRL::reject()\n");
    return false;
}

void TaskRestoreLines::changeEvent(QEvent *e)
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
    if (edgeNames.empty()) {
        taskbox->hideGroupBox();
    }

    TaskLineDecor* parent = dynamic_cast<TaskLineDecor*>(widget);
    if (parent != nullptr) {
        restore = new TaskRestoreLines(partFeat, parent);
        restoreBox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-linedecor"),
                                             tr("Restore Invisible Lines"), true, 0);
        restoreBox->groupLayout()->addWidget(restore);
        Content.push_back(restoreBox);
    }
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
//    Base::Console().Message("TDLD::accept()\n");
    widget->accept();
    return true;
}

bool TaskDlgLineDecor::reject()
{
//    Base::Console().Message("TDLD::reject()\n");
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskLineDecor.cpp>
